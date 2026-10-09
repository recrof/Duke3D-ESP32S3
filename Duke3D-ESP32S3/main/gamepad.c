/*
 *  Bluetooth LE gamepad support for the ESP32-S3 port (Xbox Wireless Controller and
 *  other HID-over-GATT pads), using the NimBLE host as a central.
 *
 *  The pad is found by scanning for HID / gamepad advertisements, bonded with
 *  "just works" pairing (the bond is kept in NVS, so later it reconnects by
 *  itself), and its input reports are decoded with the layout taken from its
 *  own HID report map, so button and axis positions are not hard-coded.
 *
 *  Sticks feed getAnalogInput(); buttons become scancode events in the same
 *  queue as the keyboard (see input.c).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include <string.h>
#include <strings.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "store/config/ble_store_config.h"
#include "keyboard.h"           // the game's scancodes (sc_*)
#include "port/keyboard.h"
#include "gamepad.h"

#define UUID_HID_SERVICE        0x1812
#define UUID_REPORT_MAP         0x2A4B
#define UUID_REPORT             0x2A4D
#define UUID_CCCD               0x2902
#define APPEARANCE_GAMEPAD      0x03C4
#define APPEARANCE_JOYSTICK     0x03C3
#define MAX_REPORT_MAP          1024
#define MAX_REPORT_CHRS         8
#define MAX_FIELDS              64
#define MAX_BUTTONS             16
#define STICK_DEADZONE          10      // out of 127
#define TRIGGER_THRESHOLD       40      // percent of travel
#define GAMEPAD_DEBUG           0

void ble_store_config_init(void);

static const char *TAG = "gamepad";

// ---------------------------------------------------------------------------
// HID report map parsing
// ---------------------------------------------------------------------------
typedef struct
{
    uint16_t usagePage;
    uint16_t usage;
    uint16_t bitOffset;         // within the report payload (report ID excluded)
    uint8_t bitSize;
    int32_t logicalMin;
    int32_t logicalMax;
    uint8_t reportId;
} hidField_t;

typedef struct
{
    hidField_t fields[MAX_FIELDS];
    int numFields;
    uint8_t padReportId;        // report that carries the X axis
    uint16_t padReportBits;
    // indexes into fields[], -1 if absent
    int lx, ly, rx, ry, lt, rt, hat;
    int buttons[MAX_BUTTONS];   // button usage 1..16
} hidLayout_t;

static hidLayout_t layout = {.lx = -1, .ly = -1, .rx = -1, .ry = -1, .lt = -1, .rt = -1, .hat = -1};

static int32_t itemValue(const uint8_t *p, int size, bool isSigned)
{
    uint32_t v = 0;
    for (int i = 0; i < size; i++)
    {
        v |= (uint32_t) p[i] << (8 * i);
    }
    if (isSigned && size && size < 4 && (v & (1u << (8 * size - 1))))
    {
        v |= ~0u << (8 * size);
    }
    return (int32_t) v;
}

static void parseReportMap(const uint8_t *map, int len)
{
    memset(&layout, 0, sizeof(layout));
    // global state (with a small push/pop stack)
    struct
    {
        uint16_t usagePage;
        int32_t logicalMin, logicalMax;
        uint8_t reportSize, reportCount, reportId;
    } g = {0}, stack[4];
    int sp = 0;
    uint16_t usages[16];
    int numUsages = 0;
    uint32_t usageMin = 0, usageMax = 0;
    bool haveRange = false;
    uint16_t inputBits[256] = {0};      // running bit offset per report ID
    for (int pos = 0; pos < len;)
    {
        uint8_t prefix = map[pos++];
        if (prefix == 0xFE)             // long item: skip
        {
            if (pos + 2 > len)
            {
                break;
            }
            pos += 2 + map[pos];
            continue;
        }
        int size = prefix & 3;
        size = size == 3 ? 4 : size;
        int type = (prefix >> 2) & 3;
        int tag = prefix >> 4;
        if (pos + size > len)
        {
            break;
        }
        const uint8_t *data = &map[pos];
        pos += size;
        uint32_t uval = (uint32_t) itemValue(data, size, false);
        if (type == 1)                  // global
        {
            switch (tag)
            {
                case 0x0: g.usagePage = uval; break;
                case 0x1: g.logicalMin = itemValue(data, size, true); break;
                case 0x2: g.logicalMax = itemValue(data, size, true); break;
                case 0x7: g.reportSize = uval; break;
                case 0x8: g.reportId = uval; break;
                case 0x9: g.reportCount = uval; break;
                case 0xA: if (sp < 4) stack[sp++] = g; break;
                case 0xB: if (sp > 0) g = stack[--sp]; break;
            }
        }
        else if (type == 2)             // local
        {
            switch (tag)
            {
                case 0x0:
                    if (numUsages < 16)
                    {
                        usages[numUsages++] = uval & 0xFFFF;
                    }
                    break;
                case 0x1: usageMin = uval & 0xFFFF; haveRange = true; break;
                case 0x2: usageMax = uval & 0xFFFF; haveRange = true; break;
            }
        }
        else if (type == 0)             // main
        {
            if (tag == 0x8)             // input
            {
                bool constant = uval & 1;
                for (int i = 0; i < g.reportCount; i++)
                {
                    uint16_t usage = 0;
                    if (numUsages)
                    {
                        usage = usages[i < numUsages ? i : numUsages - 1];
                    }
                    else if (haveRange)
                    {
                        usage = usageMin + i <= usageMax ? usageMin + i : usageMax;
                    }
                    if (!constant && layout.numFields < MAX_FIELDS && g.reportSize <= 32)
                    {
                        hidField_t *f = &layout.fields[layout.numFields++];
                        f->usagePage = g.usagePage;
                        f->usage = usage;
                        f->bitOffset = inputBits[g.reportId];
                        f->bitSize = g.reportSize;
                        f->logicalMin = g.logicalMin;
                        f->logicalMax = g.logicalMax;
                        f->reportId = g.reportId;
                    }
                    inputBits[g.reportId] += g.reportSize;
                }
            }
            // locals are reset by every main item
            numUsages = 0;
            haveRange = false;
            usageMin = usageMax = 0;
        }
    }
    // Pick the fields the game uses.
    layout.lx = layout.ly = layout.rx = layout.ry = layout.lt = layout.rt = layout.hat = -1;
    for (int i = 0; i < MAX_BUTTONS; i++)
    {
        layout.buttons[i] = -1;
    }
    int gdRx = -1, gdRy = -1;
    for (int i = 0; i < layout.numFields; i++)
    {
        hidField_t *f = &layout.fields[i];
        if (f->usagePage == 0x01)           // generic desktop
        {
            switch (f->usage)
            {
                case 0x30: if (layout.lx < 0) layout.lx = i; break;
                case 0x31: if (layout.ly < 0) layout.ly = i; break;
                case 0x32: if (layout.rx < 0) layout.rx = i; break;   // Z: right stick X (Xbox)
                case 0x35: if (layout.ry < 0) layout.ry = i; break;   // Rz: right stick Y (Xbox)
                case 0x33: if (gdRx < 0) gdRx = i; break;
                case 0x34: if (gdRy < 0) gdRy = i; break;
                case 0x39: if (layout.hat < 0) layout.hat = i; break;
            }
        }
        else if (f->usagePage == 0x02)      // simulation controls
        {
            if (f->usage == 0xC5 && layout.lt < 0) layout.lt = i;     // brake
            if (f->usage == 0xC4 && layout.rt < 0) layout.rt = i;     // accelerator
        }
        else if (f->usagePage == 0x09 && f->usage >= 1 && f->usage <= MAX_BUTTONS)
        {
            if (layout.buttons[f->usage - 1] < 0) layout.buttons[f->usage - 1] = i;
        }
    }
    // Pads without Z/Rz sticks use Rx/Ry for the right stick; otherwise Rx/Ry
    // (if present) are analog triggers.
    if (layout.rx < 0 || layout.ry < 0)
    {
        layout.rx = gdRx;
        layout.ry = gdRy;
    }
    else
    {
        if (layout.lt < 0) layout.lt = gdRx;
        if (layout.rt < 0) layout.rt = gdRy;
    }
    if (layout.lx >= 0)
    {
        layout.padReportId = layout.fields[layout.lx].reportId;
        layout.padReportBits = inputBits[layout.padReportId];
    }
    ESP_LOGI(TAG, "report map: %d fields, pad report id %d (%d bytes), sticks %d %d %d %d, triggers %d %d, hat %d",
             layout.numFields, layout.padReportId, (layout.padReportBits + 7) / 8,
             layout.lx, layout.ly, layout.rx, layout.ry, layout.lt, layout.rt, layout.hat);
}

static uint32_t fieldRaw(const hidField_t *f, const uint8_t *data, int len)
{
    uint32_t v = 0;
    for (int b = 0; b < f->bitSize; b++)
    {
        int bit = f->bitOffset + b;
        if (bit / 8 >= len)
        {
            break;
        }
        v |= (uint32_t) ((data[bit / 8] >> (bit % 8)) & 1) << b;
    }
    return v;
}

static int32_t fieldValue(const hidField_t *f, const uint8_t *data, int len)
{
    uint32_t v = fieldRaw(f, data, len);
    if (f->logicalMin < 0 && f->bitSize > 0 && f->bitSize < 32 && (v & (1u << (f->bitSize - 1))))
    {
        v |= ~0u << f->bitSize;     // sign extend
    }
    return (int32_t) v;
}

// Axis value scaled to -128..127 around the centre of its logical range.
static int32_t axisValue(int index, const uint8_t *data, int len)
{
    if (index < 0)
    {
        return 0;
    }
    const hidField_t *f = &layout.fields[index];
    int32_t min = f->logicalMin, max = f->logicalMax;
    if (max <= min)
    {
        max = (1 << f->bitSize) - 1;
        min = 0;
    }
    if (max <= min)
    {
        return 0;
    }
    int32_t v = fieldValue(f, data, len);
    int32_t scaled = (int32_t) (((int64_t) (v - min) * 255) / (max - min)) - 128;
    if (scaled > -STICK_DEADZONE && scaled < STICK_DEADZONE)
    {
        return 0;
    }
    return scaled > 127 ? 127 : (scaled < -128 ? -128 : scaled);
}

// Trigger travel in percent.
static int triggerValue(int index, const uint8_t *data, int len)
{
    if (index < 0)
    {
        return 0;
    }
    const hidField_t *f = &layout.fields[index];
    int32_t max = f->logicalMax > f->logicalMin ? f->logicalMax : (1 << f->bitSize) - 1;
    if (max <= f->logicalMin)
    {
        return 0;
    }
    int32_t v = fieldValue(f, data, len) - f->logicalMin;
    return (int) ((int64_t) v * 100 / (max - f->logicalMin));
}

// ---------------------------------------------------------------------------
// Pad state -> game input
// ---------------------------------------------------------------------------
enum
{
    PB_A, PB_B, PB_X, PB_Y, PB_LB, PB_RB, PB_VIEW, PB_MENU, PB_GUIDE, PB_LS, PB_RS,
    PB_LT, PB_RT, PB_UP, PB_DOWN, PB_LEFT, PB_RIGHT, PB_COUNT
};
// Xbox Wireless Controller HID button numbers (1-based) for the face/shoulder
// buttons; standard "gamepad" HID layouts number them the same way up to 8.
static const int8_t xboxButtonUsage[PB_COUNT] =
{
    [PB_A] = 1, [PB_B] = 2, [PB_X] = 4, [PB_Y] = 5, [PB_LB] = 7, [PB_RB] = 8,
    [PB_VIEW] = 11, [PB_MENU] = 12, [PB_GUIDE] = 13, [PB_LS] = 14, [PB_RS] = 15,
    [PB_LT] = 0, [PB_RT] = 0, [PB_UP] = 0, [PB_DOWN] = 0, [PB_LEFT] = 0, [PB_RIGHT] = 0,
};
static volatile int8_t axes[4];         // lx, ly, rx, ry
static bool pressed[PB_COUNT];
static uint8_t pressedScancode[PB_COUNT];
static volatile bool connected;

static uint8_t buttonScancode(int b, bool menu)
{
    if (menu)
    {
        switch (b)
        {
            case PB_A: return sc_Enter;
            case PB_B: return sc_Escape;
            default: break;
        }
    }
    switch (b)
    {
        case PB_A: return sc_Slash;             // jump
        case PB_B: return sc_Z;                 // crouch
        case PB_X: return sc_Space;             // open / use
        case PB_Y: return sc_kpad_Enter;        // use inventory item
        case PB_LB: return sc_SemiColon;        // previous weapon
        case PB_RB: return sc_Quote;            // next weapon
        case PB_VIEW: return sc_Tab;            // map
        case PB_MENU: return sc_Escape;
        case PB_GUIDE: return sc_Escape;
        case PB_LS: return sc_C;                // quick kick
        case PB_RS: return sc_kpad_5;           // centre view
        case PB_LT: return sc_RightShift;       // run
        case PB_RT: return sc_RightControl;     // fire
        case PB_UP: return sc_UpArrow;
        case PB_DOWN: return sc_DownArrow;
        case PB_LEFT: return sc_LeftArrow;
        case PB_RIGHT: return sc_RightArrow;
        default: return 0;
    }
}

static void setButton(int b, bool down)
{
    if (down == pressed[b])
    {
        return;
    }
    pressed[b] = down;
    if (down)
    {
        pressedScancode[b] = buttonScancode(b, inputIsMenuMode());
        inputPostKeyEvent(pressedScancode[b], true);
    }
    else
    {
        inputPostKeyEvent(pressedScancode[b], false);
    }
}

static void releaseAll(void)
{
    for (int b = 0; b < PB_COUNT; b++)
    {
        setButton(b, false);
    }
    for (int i = 0; i < 4; i++)
    {
        axes[i] = 0;
    }
}

static void handleReport(const uint8_t *data, int len)
{
    // Pads start sending as soon as the link is encrypted, before the report
    // map has been read: ignore those reports.
    if (!connected || layout.lx < 0 || len < (layout.padReportBits + 7) / 8)
    {
        return;
    }
    axes[0] = axisValue(layout.lx, data, len);
    axes[1] = axisValue(layout.ly, data, len);
    axes[2] = axisValue(layout.rx, data, len);
    axes[3] = axisValue(layout.ry, data, len);
    for (int b = 0; b < PB_COUNT; b++)
    {
        int usage = xboxButtonUsage[b];
        if (usage > 0 && layout.buttons[usage - 1] >= 0)
        {
            setButton(b, fieldRaw(&layout.fields[layout.buttons[usage - 1]], data, len) != 0);
        }
    }
    setButton(PB_LT, triggerValue(layout.lt, data, len) > TRIGGER_THRESHOLD);
    setButton(PB_RT, triggerValue(layout.rt, data, len) > TRIGGER_THRESHOLD);
    bool up = false, down = false, left = false, right = false;
    if (layout.hat >= 0)
    {
        const hidField_t *f = &layout.fields[layout.hat];
        int32_t v = fieldValue(f, data, len);
        if (v >= f->logicalMin && v <= f->logicalMax && f->logicalMax - f->logicalMin == 7)
        {
            int dir = v - f->logicalMin;        // 0 = up, clockwise in 45 degree steps
            up = dir == 7 || dir <= 1;
            right = dir >= 1 && dir <= 3;
            down = dir >= 3 && dir <= 5;
            left = dir >= 5;
        }
    }
    setButton(PB_UP, up);
    setButton(PB_DOWN, down);
    setButton(PB_LEFT, left);
    setButton(PB_RIGHT, right);
}

void gamepadGetAxes(int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry)
{
    *lx = axes[0];
    *ly = axes[1];
    *rx = axes[2];
    *ry = axes[3];
}

bool gamepadConnected(void)
{
    return connected;
}

// ---------------------------------------------------------------------------
// BLE central: scan, connect, bond, discover, subscribe
// ---------------------------------------------------------------------------
typedef struct
{
    uint16_t valHandle;
    uint16_t cccdHandle;
    bool notify;
} reportChr_t;

static uint8_t ownAddrType;
static uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t svcStart, svcEnd;
static uint16_t reportMapHandle;
static reportChr_t reports[MAX_REPORT_CHRS];
static int numReports;
static int subscribeIndex;
static uint8_t reportMap[MAX_REPORT_MAP];
static int reportMapLen;

static void startScan(void);
static int gapEvent(struct ble_gap_event *event, void *arg);

static void fail(const char *what, int rc)
{
    ESP_LOGW(TAG, "%s failed (%d), disconnecting", what, rc);
    if (connHandle != BLE_HS_CONN_HANDLE_NONE)
    {
        ble_gap_terminate(connHandle, BLE_ERR_REM_USER_CONN_TERM);
    }
}

static int onSubscribed(uint16_t conn, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg);

static void subscribeNext(void)
{
    while (subscribeIndex < numReports && !(reports[subscribeIndex].notify && reports[subscribeIndex].cccdHandle))
    {
        subscribeIndex++;
    }
    if (subscribeIndex >= numReports)
    {
        connected = true;
        ESP_LOGI(TAG, "controller ready");
        return;
    }
    static const uint8_t enable[2] = {1, 0};
    int rc = ble_gattc_write_flat(connHandle, reports[subscribeIndex].cccdHandle, enable, sizeof(enable), onSubscribed, NULL);
    if (rc)
    {
        fail("subscribe", rc);
    }
}

static int onSubscribed(uint16_t conn, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    if (error->status)
    {
        ESP_LOGW(TAG, "CCCD write on 0x%04x: %d", reports[subscribeIndex].cccdHandle, error->status);
    }
    subscribeIndex++;
    subscribeNext();
    return 0;
}

static int onReportMap(uint16_t conn, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    if (error->status == 0 && attr)
    {
        int n = OS_MBUF_PKTLEN(attr->om);
        if (reportMapLen + n > MAX_REPORT_MAP)
        {
            n = MAX_REPORT_MAP - reportMapLen;
        }
        os_mbuf_copydata(attr->om, 0, n, reportMap + reportMapLen);
        reportMapLen += n;
        return 0;
    }
    if (error->status != BLE_HS_EDONE)
    {
        fail("report map read", error->status);
        return 0;
    }
    ESP_LOGI(TAG, "report map %d bytes", reportMapLen);
    parseReportMap(reportMap, reportMapLen);
    subscribeIndex = 0;
    subscribeNext();
    return 0;
}

static int onDescriptor(uint16_t conn, const struct ble_gatt_error *error, uint16_t chrValHandle,
                        const struct ble_gatt_dsc *dsc, void *arg)
{
    if (error->status == 0)
    {
        if (ble_uuid_cmp(&dsc->uuid.u, BLE_UUID16_DECLARE(UUID_CCCD)) == 0)
        {
            for (int i = 0; i < numReports; i++)
            {
                if (reports[i].valHandle == chrValHandle)
                {
                    reports[i].cccdHandle = dsc->handle;
                }
            }
        }
        return 0;
    }
    if (error->status != BLE_HS_EDONE)
    {
        fail("descriptor discovery", error->status);
        return 0;
    }
    if (!reportMapHandle)
    {
        fail("no report map", 0);
        return 0;
    }
    reportMapLen = 0;
    int rc = ble_gattc_read_long(connHandle, reportMapHandle, 0, onReportMap, NULL);
    if (rc)
    {
        fail("report map read", rc);
    }
    return 0;
}

static int onCharacteristic(uint16_t conn, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg)
{
    if (error->status == 0)
    {
        if (ble_uuid_cmp(&chr->uuid.u, BLE_UUID16_DECLARE(UUID_REPORT_MAP)) == 0)
        {
            reportMapHandle = chr->val_handle;
        }
        else if (ble_uuid_cmp(&chr->uuid.u, BLE_UUID16_DECLARE(UUID_REPORT)) == 0 && numReports < MAX_REPORT_CHRS)
        {
            reports[numReports].valHandle = chr->val_handle;
            reports[numReports].cccdHandle = 0;
            reports[numReports].notify = (chr->properties & BLE_GATT_CHR_PROP_NOTIFY) != 0;
            numReports++;
        }
        return 0;
    }
    if (error->status != BLE_HS_EDONE)
    {
        fail("characteristic discovery", error->status);
        return 0;
    }
    int rc = ble_gattc_disc_all_dscs(connHandle, svcStart, svcEnd, onDescriptor, NULL);
    if (rc)
    {
        fail("descriptor discovery", rc);
    }
    return 0;
}

static int onService(uint16_t conn, const struct ble_gatt_error *error, const struct ble_gatt_svc *svc, void *arg)
{
    if (error->status == 0)
    {
        svcStart = svc->start_handle;
        svcEnd = svc->end_handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE || !svcStart)
    {
        fail("HID service discovery", error->status);
        return 0;
    }
    numReports = 0;
    reportMapHandle = 0;
    int rc = ble_gattc_disc_all_chrs(connHandle, svcStart, svcEnd, onCharacteristic, NULL);
    if (rc)
    {
        fail("characteristic discovery", rc);
    }
    return 0;
}

static void startDiscovery(void)
{
    svcStart = svcEnd = 0;
    int rc = ble_gattc_disc_svc_by_uuid(connHandle, BLE_UUID16_DECLARE(UUID_HID_SERVICE), onService, NULL);
    if (rc)
    {
        fail("service discovery", rc);
    }
}

static bool padName(const struct ble_hs_adv_fields *fields)
{
    char name[32] = {0};
    int n = fields->name_len < sizeof(name) - 1 ? fields->name_len : sizeof(name) - 1;
    memcpy(name, fields->name, n);
    return n && (strcasestr(name, "controller") || strcasestr(name, "gamepad") || strcasestr(name, "xbox") ||
                 strcasestr(name, "8bitdo") || strcasestr(name, "pad"));
}

static bool looksLikeGamepad(const struct ble_gap_disc_desc *d)
{
    if (d->event_type == BLE_HCI_ADV_RPT_EVTYPE_DIR_IND)
    {
        return true;    // a bonded pad reconnecting to us
    }
    if (d->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND && d->event_type != BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP)
    {
        return false;
    }
    struct ble_hs_adv_fields fields;
    if (ble_hs_adv_parse_fields(&fields, d->data, d->length_data) != 0)
    {
        return false;
    }
#if GAMEPAD_DEBUG
    {
        char name[32] = {0};
        memcpy(name, fields.name, fields.name_len < 31 ? fields.name_len : 31);
        ESP_LOGI(TAG, "adv %02x:%02x:%02x:%02x:%02x:%02x type %d rssi %d name '%s' appearance %s0x%04x uuids16 %d",
                 d->addr.val[5], d->addr.val[4], d->addr.val[3], d->addr.val[2], d->addr.val[1], d->addr.val[0],
                 d->event_type, d->rssi, name, fields.appearance_is_present ? "" : "(none) ", fields.appearance,
                 fields.num_uuids16);
    }
#endif
    if (fields.appearance_is_present &&
        (fields.appearance == APPEARANCE_GAMEPAD || fields.appearance == APPEARANCE_JOYSTICK))
    {
        return true;
    }
    // The name is often only in the scan response.
    if (padName(&fields))
    {
        return true;
    }
    for (int i = 0; i < fields.num_uuids16; i++)
    {
        // HID alone could be a keyboard or mouse: only take it with no name.
        if (ble_uuid_u16(&fields.uuids16[i].u) == UUID_HID_SERVICE && fields.name_len == 0)
        {
            return true;
        }
    }
    return false;
}

static int gapEvent(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
        case BLE_GAP_EVENT_DISC:
            if (looksLikeGamepad(&event->disc))
            {
                ESP_LOGI(TAG, "found pad %02x:%02x:%02x:%02x:%02x:%02x, connecting",
                         event->disc.addr.val[5], event->disc.addr.val[4], event->disc.addr.val[3],
                         event->disc.addr.val[2], event->disc.addr.val[1], event->disc.addr.val[0]);
                ble_gap_disc_cancel();
                struct ble_gap_conn_params params =
                {
                    .scan_itvl = 0x0010, .scan_window = 0x0010,
                    .itvl_min = 6, .itvl_max = 12,          // 7.5..15 ms: low input latency
                    .latency = 0, .supervision_timeout = 400,
                    .min_ce_len = 0, .max_ce_len = 0,
                };
                int rc = ble_gap_connect(ownAddrType, &event->disc.addr, 10000, &params, gapEvent, NULL);
                if (rc)
                {
                    ESP_LOGW(TAG, "connect: %d", rc);
                    startScan();
                }
            }
            return 0;
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status != 0)
            {
                ESP_LOGW(TAG, "connection failed: %d", event->connect.status);
                startScan();
                return 0;
            }
            connHandle = event->connect.conn_handle;
            layout.lx = -1;     // until this pad's report map is parsed
            ESP_LOGI(TAG, "connected, securing link");
            ble_gap_security_initiate(connHandle);
            return 0;
        case BLE_GAP_EVENT_ENC_CHANGE:
            if (event->enc_change.status != 0)
            {
                // Stale bond (e.g. the pad was paired elsewhere): forget it and retry.
                ESP_LOGW(TAG, "encryption failed: %d, clearing bond", event->enc_change.status);
                struct ble_gap_conn_desc desc;
                if (ble_gap_conn_find(event->enc_change.conn_handle, &desc) == 0)
                {
                    ble_store_util_delete_peer(&desc.peer_id_addr);
                }
                ble_gap_terminate(event->enc_change.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                return 0;
            }
            ESP_LOGI(TAG, "link encrypted, discovering HID service");
            startDiscovery();
            return 0;
        case BLE_GAP_EVENT_REPEAT_PAIRING:
        {
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0)
            {
                ble_store_util_delete_peer(&desc.peer_id_addr);
            }
            return BLE_GAP_REPEAT_PAIRING_RETRY;
        }
        case BLE_GAP_EVENT_NOTIFY_RX:
        {
            uint8_t data[64];
            int len = OS_MBUF_PKTLEN(event->notify_rx.om);
            if (len > (int) sizeof(data))
            {
                len = sizeof(data);
            }
            os_mbuf_copydata(event->notify_rx.om, 0, len, data);
#if GAMEPAD_DEBUG
            static int logged;
            if (logged < 5)
            {
                logged++;
                char hex[3 * 64 + 1];
                for (int i = 0; i < len; i++)
                {
                    snprintf(hex + 3 * i, 4, "%02x ", data[i]);
                }
                ESP_LOGI(TAG, "report 0x%04x (%d): %s", event->notify_rx.attr_handle, len, hex);
            }
#endif
            handleReport(data, len);
            return 0;
        }
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "disconnected (reason %d)", event->disconnect.reason);
            connected = false;
            connHandle = BLE_HS_CONN_HANDLE_NONE;
            releaseAll();
            startScan();
            return 0;
        case BLE_GAP_EVENT_DISC_COMPLETE:
            if (connHandle == BLE_HS_CONN_HANDLE_NONE)
            {
                startScan();
            }
            return 0;
        default:
            return 0;
    }
}

static void startScan(void)
{
    struct ble_gap_disc_params params =
    {
        .itvl = 0x0050,         // 50 ms
        .window = 0x0030,       // 30 ms: leave the radio some idle time
        .filter_policy = 0,
        .limited = 0,
        .passive = 0,           // active: names are often in the scan response
        .filter_duplicates = 1,
    };
    int rc = ble_gap_disc(ownAddrType, BLE_HS_FOREVER, &params, gapEvent, NULL);
    if (rc && rc != BLE_HS_EALREADY)
    {
        ESP_LOGW(TAG, "scan start: %d", rc);
    }
}

static void onSync(void)
{
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, &ownAddrType);
    ESP_LOGI(TAG, "scanning for a Bluetooth LE controller");
    startScan();
}

static void onReset(int reason)
{
    ESP_LOGW(TAG, "BLE host reset: %d", reason);
}

static void hostTask(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void gamepadInit(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK || nimble_port_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Bluetooth init failed");
        return;
    }
    ble_hs_cfg.sync_cb = onSync;
    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_store_config_init();
    nimble_port_freertos_init(hostTask);
}
