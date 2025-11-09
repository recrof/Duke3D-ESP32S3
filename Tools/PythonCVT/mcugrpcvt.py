import mido
import io
import struct
import argparse
import struct

# Import the file list from duke3d_file_list.py
# otherwise use this builtin list, which is the original Duke3D.GRP file list and it is put here so that the script can be used standalone
try:
    from duke13d_file_list import file_list as original_file_list
except ImportError:
    original_file_list = [
    'LOGO.ANM',
    'CINEOV2.ANM',
    'CINEOV3.ANM',
    'DUKETEAM.ANM',
    'RADLOGO.ANM',
    'DEFS.CON',
    'GAME.CON',
    'USER.CON',
    'D3DTIMBR.TMB',
    'DUKE3D.BIN',
    'LOOKUP.DAT',
    'PALETTE.DAT',
    'TABLES.DAT',
    'FLYBY.VOC',
    'MUSTDIE.VOC',
    'DEFEATED.VOC',
    'MONOLITH.VOC',
    'HYDRO50.VOC',
    'ASWTCH23.VOC',
    'HYDRO22.VOC',
    'HYDRO24.VOC',
    'HYDRO27.VOC',
    'HYDRO34.VOC',
    'HYDRO40.VOC',
    'HYDRO43.VOC',
    'VAULT04.VOC',
    '2BWILD.VOC',
    'FREEZE.VOC',
    'B2ATK02.VOC',
    'B2ATK01.VOC',
    'B2DIE03.VOC',
    'B2PAIN03.VOC',
    'B2REC03.VOC',
    'B3ROAM01.VOC',
    'B3DIE03G.VOC',
    'B3REC03G.VOC',
    'B3PAIN04.VOC',
    'CTRLRM25.VOC',
    'HLIDLE03.VOC',
    'LIZSHIT3.VOC',
    'ADOOR1.VOC',
    'ADOOR2.VOC',
    'FORCE01.VOC',
    'JONES04.VOC',
    'QUAKE06.VOC',
    'TERMIN01.VOC',
    'BORN01.VOC',
    'CON03.VOC',
    'SNAKRM.VOC',
    'SNAKRG.VOC',
    'SNAKATA.VOC',
    'SNAKATB.VOC',
    'SNAKPN.VOC',
    'SNAKDY.VOC',
    'COMMRM.VOC',
    'COMMRG.VOC',
    'COMMAT.VOC',
    'COMMPN.VOC',
    'COMMDY.VOC',
    'COMMSP.VOC',
    'SLIMAT.VOC',
    'SLHTCH01.VOC',
    'SLIDIE03.VOC',
    'SLIREC06.VOC',
    'SLIROA02.VOC',
    'KTITX.VOC',
    'WHIPYU01.VOC',
    'GUNHIT2.VOC',
    'HEADRIP3.VOC',
    'BUCKLE.VOC',
    'JETP2.VOC',
    'ZIPPER2.VOC',
    'NEWS.VOC',
    'WHISTLE.VOC',
    'CHEER.VOC',
    'GMEOVR05.VOC',
    'GRABBAG.VOC',
    'KICKHEAD.VOC',
    'NAME01.VOC',
    'R&R01.VOC',
    'LANI08.VOC',
    'LANI05.VOC',
    'LANIDUK2.VOC',
    'SECRET.VOC',
    'BLANK.VOC',
    'ROAM06.VOC',
    'ROAM58.VOC',
    'PREDRG.VOC',
    'GBLASR01.VOC',
    'PREDPN.VOC',
    'PREDDY.VOC',
    'CHOKN12.VOC',
    'PREDRM.VOC',
    'LIZSPIT.VOC',
    'PIGRM.VOC',
    'ROAM29.VOC',
    'ROAM67.VOC',
    'PIGRG.VOC',
    'PIGPN.VOC',
    'PIGDY.VOC',
    'PIGWRN.VOC',
    'OCTARM.VOC',
    'OCTARG.VOC',
    'OCTAAT1.VOC',
    'OCTAAT2.VOC',
    'OCTAPN.VOC',
    'OCTADY.VOC',
    'BOS1RM.VOC',
    'BOS1RG.VOC',
    'BOS1PN.VOC',
    'BOS1DY.VOC',
    'KICKHIT.VOC',
    'RICOCHET.VOC',
    'BULITHIT.VOC',
    'PISTOL.VOC',
    'CLIPOUT.VOC',
    'CLIPIN.VOC',
    'CHAINGUN.VOC',
    'SHOTGNCK.VOC',
    'RPGFIRE.VOC',
    'BOMBEXPL.VOC',
    'PBOMBBNC.VOC',
    'WPNSEL21.VOC',
    'SHRINK.VOC',
    'LSRBMBPT.VOC',
    'LSRBMBWN.VOC',
    'SHRINKER.VOC',
    'VENTBUST.VOC',
    'GLASS.VOC',
    'GLASHEVY.VOC',
    'SHORTED.VOC',
    'SPLASH.VOC',
    'ALERT.VOC',
    'REACTOR.VOC',
    'SUBWAY.VOC',
    'GEARGRND.VOC',
    'GASP.VOC',
    'GASPS07.VOC',
    'PISSING.VOC',
    'KNUCKLE.VOC',
    'DRINK18.VOC',
    'EXERT.VOC',
    'HARTBEAT.VOC',
    'PAIN13.VOC',
    'PAIN28.VOC',
    'PAIN39.VOC',
    'PAIN87.VOC',
    'WETFEET.VOC',
    'LAND02.VOC',
    'DUCTWLK.VOC',
    'PAIN54.VOC',
    'PAIN75.VOC',
    'PAIN93.VOC',
    'PAIN68.VOC',
    'DAMN03.VOC',
    'DAMNIT04.VOC',
    'COMEON02.VOC',
    'WAITIN03.VOC',
    'COOL01.VOC',
    'AHMUCH03.VOC',
    'DANCE01.VOC',
    'LETSRK03.VOC',
    'READY2A.VOC',
    'RIPEM08.VOC',
    'ROCKIN02.VOC',
    'AHH04.VOC',
    'GULP01.VOC',
    'PAY02.VOC',
    'AMESS06.VOC',
    'BITCHN04.VOC',
    'DOOMED16.VOC',
    'HOLYCW01.VOC',
    'HOLYSH02.VOC',
    'IMGOOD12.VOC',
    'ONLYON03.VOC',
    'PIECE02.VOC',
    'RIDES09.VOC',
    '2RIDE06.VOC',
    'THSUK13A.VOC',
    'WANSOM4A.VOC',
    'MYSELF3A.VOC',
    'NEEDED03.VOC',
    'SHAKE2A.VOC',
    'DUKNUK14.VOC',
    'GETSOM1A.VOC',
    'GOTHRT01.VOC',
    'GROOVY02.VOC',
    'WHRSIT05.VOC',
    'BOOBY04.VOC',
    'DIESOB03.VOC',
    'DSCREM04.VOC',
    'LOOKIN01.VOC',
    'PISSIN01.VOC',
    'GETITM19.VOC',
    'SCUBA.VOC',
    'JETPAKON.VOC',
    'JETPAKI.VOC',
    'JETPAKOF.VOC',
    'GOGGLE12.VOC',
    'THUD.VOC',
    'SQUISH.VOC',
    'TELEPORT.VOC',
    'GBELEV01.VOC',
    'GBELEV02.VOC',
    'SWITCH.VOC',
    'FLUSH.VOC',
    'QUAKE.VOC',
    'MONITOR.VOC',
    'POOLBAL1.VOC',
    'ONBOARD.VOC',
    'BUBBLAMB.VOC',
    'MACHAMB.VOC',
    'WIND54.VOC',
    'STEAMHIS.VOC',
    'BARMUSIC.VOC',
    'WARAMB13.VOC',
    'WARAMB21.VOC',
    'WARAMB23.VOC',
    'WARAMB29.VOC',
    'COMPAMB.VOC',
    'SLIDOOR.VOC',
    'OPENDOOR.VOC',
    'EDOOR10.VOC',
    'EDOOR11.VOC',
    'FSCRM10.VOC',
    'H2OGRGL2.VOC',
    'GRIND.VOC',
    'ENGHUM.VOC',
    'LAVA06.VOC',
    'PHONBUSY.VOC',
    'ROAM22.VOC',
    'AMB81B.VOC',
    'ROAM98B.VOC',
    'H2ORUSH2.VOC',
    'PROJRUN.VOC',
    'FIRE09.VOC',
    '!PRISON.VOC',
    '!PIG.VOC',
    '!BOSS.VOC',
    'MICE3.VOC',
    'DRIP3.VOC',
    'ITEM15.VOC',
    'BONUS.VOC',
    'CATFIRE.VOC',
    'KILLME.VOC',
    'SHOTGUN7.VOC',
    'DMDEATH.VOC',
    'HAPPEN01.VOC',
    'DSCREM15.VOC',
    'DSCREM16.VOC',
    'DSCREM17.VOC',
    'DSCREM18.VOC',
    'DSCREM38.VOC',
    'RIP01.VOC',
    'NOBODY01.VOC',
    'CHEW05.VOC',
    'LETGOD01.VOC',
    'HAIL01.VOC',
    'BLOWIT01.VOC',
    'EATSHT01.VOC',
    'FACE01.VOC',
    'INHELL01.VOC',
    'SUKIT01.VOC',
    'PISSES01.VOC',
    'STORM.MID',
    'FATCMDR.MID',
    'GUTWRNCH.MID',
    'NAMES.MID',
    'GLOOMY.MID',
    'FUTURMIL.MID',
    'INTENTS.MID',
    'INVADER.MID',
    'SUBWAY.MID',
    'AHGEEZ.MID',
    '233C.MID',
    'ALFREDH.MID',
    'ALIENZ.MID',
    'GOTHAM.MID',
    'INHIDING.MID',
    'LORDOFLA.MID',
    'PIZZED.MID',
    'ROBOCREP.MID',
    'SPOOK.MID',
    'STALAG.MID',
    'URBAN.MID',
    'WHOMP.MID',
    'XPLASMA.MID',
    'GRABBAG.MID',
    'STALKER.MID',
    'DETHTOLL.MID',
    'STREETS.MID',
    'WATRWLD1.MID',
    'SNAKE1.MID',
    'THECALL.MID',
    'TILES000.ART',
    'TILES001.ART',
    'TILES002.ART',
    'TILES003.ART',
    'TILES004.ART',
    'TILES005.ART',
    'TILES006.ART',
    'TILES007.ART',
    'TILES008.ART',
    'TILES009.ART',
    'TILES010.ART',
    'TILES011.ART',
    'TILES012.ART',
    'TILES013.ART',
    'TILES014.ART',
    'E1L7.MAP',
    'E1L8.MAP',
    'E2L1.MAP',
    'E2L2.MAP',
    'E2L3.MAP',
    'E2L4.MAP',
    'E2L5.MAP',
    'E2L6.MAP',
    'E2L7.MAP',
    'E2L8.MAP',
    'E2L9.MAP',
    'E2L10.MAP',
    'E2L11.MAP',
    'E3L1.MAP',
    'E3L2.MAP',
    'E3L3.MAP',
    'E3L4.MAP',
    'E3L5.MAP',
    'E3L6.MAP',
    'E3L7.MAP',
    'E3L8.MAP',
    'E3L9.MAP',
    'E3L10.MAP',
    'E3L11.MAP',
    'E1L1.MAP',
    'E1L2.MAP',
    'E1L3.MAP',
    'E1L4.MAP',
    'E1L5.MAP',
    'E1L6.MAP',
]


# Create a modified file list with .voc files renamed to .wav
file_list = [filename[:-4] + '.wav' if filename.lower().endswith('.voc') else filename for filename in original_file_list]

def decode_voc_to_wav(voc_data):
    # Read VOC header
    voc_io = io.BytesIO(voc_data)
    header = voc_io.read(26)
    bits_per_sample = 8
    number_of_block9 = 0
    if header[:20] != b'Creative Voice File\x1A':
        raise ValueError("Not a valid VOC file. header is: " + header[:20].decode(encoding = "utf-8", errors = "ignore"))

    print("CONVERTING")

    # Read blocks
    audio_data = bytearray()
    while True:
        block_type = voc_io.read(1)
        if not block_type:
            break
        block_type = block_type[0]
        if block_type == 1:  # Sound data block
            block_size = struct.unpack('<H', voc_io.read(2))[0] + 65536 * voc_io.read(1)[0]
            sample_rate = 1000000 // (256 - voc_io.read(1)[0])
            compression = voc_io.read(1)[0]
            if compression != 0:
                raise ValueError("Unsupported compression type")
            audio_data.extend(voc_io.read(block_size - 2))
        elif block_type == 2:  # Continue Sound data block
            block_size = struct.unpack('<H', voc_io.read(2))[0] + 65536 * voc_io.read(1)[0]
            audio_data.extend(voc_io.read(block_size))
        elif block_type == 3:  # Silence
            block_size = struct.unpack('<H', voc_io.read(2))[0] + 65536 * voc_io.read(1)[0]
            raise ValueError("BLOCK 3 NOT SUPPORTED")
        elif block_type == 4:  # Marker
            block_size = struct.unpack('<H', voc_io.read(2))[0] + 65536 * voc_io.read(1)[0]
            raise ValueError("BLOCK 4 NOT SUPPORTED")
        elif block_type == 5:  # string
            block_size = struct.unpack('<H', voc_io.read(2))[0] + 65536 * voc_io.read(1)[0]
            #discard string
            voc_io.read(block_size)     #just read and discard
        elif block_type > 5 and block_type < 9:  # repeat and extended
            raise ValueError("BLOCK " + str(block_type) + " NOT SUPPORTED")
        elif block_type == 9:
            number_of_block9 += 1
            if number_of_block9 > 1:
                raise ValueError("TOO MANY BLOCKS TYPE 9")
            block_size = struct.unpack('<H', voc_io.read(2))[0] + 65536 * voc_io.read(1)[0]
            sample_rate = struct.unpack('<I', voc_io.read(4))[0]
            print("Sample rate = " + str(sample_rate));
            bits_per_sample = voc_io.read(1)[0]
            print("bits_per_sample = " + str(bits_per_sample))
            if (bits_per_sample != 8 and bits_per_sample != 16 and bits_per_sample != 0):
                raise ValueError( "unsupported bits per sample " + str(bits_per_sample))
            channels = voc_io.read(1)[0]
            print("channels = " + str(channels))
            if (channels > 1):
                raise ValueError("Too many channels")
            fmt = struct.unpack('<H', voc_io.read(2))[0]
            print("format: " + str(fmt))
            if fmt != 4 and fmt != 0:
                raise ValueError("Unsupported format type")
            paddings = voc_io.read(4)    # discard padding bytes
            newbytearray = voc_io.read(block_size - 12)
            if (bits_per_sample == 8):
                audio_data.extend(newbytearray)
            elif (bits_per_sample == 16):
                #audio_data.extend(newbytearray[1::2])
                #print("Converted 16 to 8 bits")
                # Convert 16-bit signed to 8-bit unsigned
                for i in range(0, len(newbytearray), 2):
                    sample_16bit = struct.unpack('<h', newbytearray[i:i+2])[0]
                    sample_8bit = (sample_16bit >> 8) + 128
                    audio_data.append(sample_8bit)
                print("Converted 16-bit signed to 8-bit unsigned")
            else:
                # SNAKRM.VOC is corrupted. Let's not add anything.  
                print("Corrupted file. Whatever...")                              
        elif block_type == 0:       # terminator
            break
        elif block_type > 9 or block_type == 6 or block_type == 7:
            raise ValueError("Unsupported block type " + str(block_type))
    # Create WAV header
    num_channels = 1
    byte_rate = sample_rate * num_channels * 1  # 8-bit audio
    block_align = num_channels * 1
    subchunk2_size = len(audio_data)
    chunk_size = 36 + subchunk2_size

    wav_header = struct.pack('<4sI4s4sIHHIIHH4sI',
                             b'RIFF',
                             chunk_size,
                             b'WAVE',
                             b'fmt ',
                             16,
                             1,
                             num_channels,
                             sample_rate,
                             byte_rate,
                             block_align,
                             bits_per_sample,
                             b'data',
                             subchunk2_size)

    return wav_header + audio_data
        
def create_grp_file(output_path, file_table, file_data, use_file_list = False):
    # Create the GRP header
    signature = b"KenSilverman"
    num_files = len(file_table)
    
    # Update sizes in the file table
    updated_file_table = []

    real_num_files = 0    
    found_files = set()

    for idx in range(num_files):
        if use_file_list and file_table[idx][0] not in file_list:
            print("skipping ", file_table[idx][0], " as not in list")
            continue
        file_name = file_table[idx][0]
        file_size = len(file_data[idx])
        updated_file_table.append((file_name, file_size))
        found_files.add(file_name)
        real_num_files += 1
    
    # Check for missing files
    if use_file_list:
        missing_files = set(file_list) - found_files
        if missing_files:
            print("The following files were not found in the GRP file:")
            for missing_file in missing_files:
                print(missing_file)

    # Write the GRP file
    with open(output_path, 'wb') as grp_file:
        # Write the signature
        grp_file.write(signature)
        
        # Write the number of files
        grp_file.write(struct.pack('<I', real_num_files))
        
        # Write the file table
        for file_name, file_size in updated_file_table:
            file_name_bytes = file_name.encode('ascii')
            file_name_bytes = file_name_bytes.ljust(12, b'\0')  # Pad to 12 bytes
            grp_file.write(file_name_bytes)
            grp_file.write(struct.pack('<I', file_size))
        
        # Write the file data
        for idx in range(num_files):
            if use_file_list and file_table[idx][0] not in file_list:
                print("skipping ", file_table[idx][0], " as not in list")
                continue            
            grp_file.write(file_data[idx])
    
    print(f'Created GRP file: {output_path}')



def convert_grp_file(file_path, new_file_path=str, convert_voc_to_wav=True, convert_midi_to_type0=True, downgrade=False):
    with open(file_path, 'rb') as f:
        # Read the header (example: first 12 bytes)
        header = f.read(16)
        signature = header[:12]
        print(signature)
        if (signature != b'KenSilverman'):
            print("Wrong GRP Header, exiting")
            exit()
        num_files = struct.unpack('<I', header[12:16])[0]
        
        print(f'Signature: {signature}')
        print(f'Number of files: {num_files}')

        # Read the file table
        file_table = []
        file_offset = 16 + num_files * 16
        for _ in range(num_files):
            entry = f.read(16)  
            file_name = entry[:12].decode('ascii').strip('\x00')
            file_size = struct.unpack('<I', entry[12:16])[0]
            file_table.append((file_name, file_size, file_offset))
            file_offset = file_offset + file_size
        
        # Print the file table
        #for file_name, file_size, file_offset in file_table:
        #    print(f'File: {file_name}, Size: {file_size} bytes, Offset: {file_offset}')

        file_data = []

        for idx in range(num_files):
            f.seek(file_table[idx][2], 0)
            print("reading file: " + file_table[idx][0] + " at offset: " + str(file_table[idx][2]) + " size: " + str(file_table[idx][1]))
            file_data.append(f.read(file_table[idx][1]))

        # now that we have all the files in an array, we want to convert the voc to wav, if needed
        if convert_voc_to_wav:
            for idx in range(num_files):
                if file_table[idx][0].lower().endswith(".voc"):
                    print("converting voc " + file_table[idx][0] +  " to wav. Index is " + str(idx))
                    temp_list = list(file_table[idx])
                    temp_list[0] = file_table[idx][0][:-3] + "wav"
                    file_table[idx] = tuple(temp_list)
                    print(file_table[idx][0])
                    file_data[idx] = decode_voc_to_wav(file_data[idx])
                    # update file size
                    temp_list = list(file_table[idx])
                    temp_list[1] = len(file_data[idx])
                    file_table[idx] = tuple(temp_list)

        # Save the converted files to disk
        if 0:
            for idx in range(num_files):
                if file_table[idx][0].lower().endswith(".wav"):
                    output_file_path = file_table[idx][0]
                    with open(output_file_path, 'wb') as output_file:
                        output_file.write(file_data[idx])
                    print(f'Saved file: {output_file_path}')

        # Convert MIDI files from type 1 to type 0
        if convert_midi_to_type0:
            for idx in range(num_files):
                if file_table[idx][0].lower().endswith(".mid"):
                    print("converting midi " + file_table[idx][0] + " to type 0. Index is " + str(idx))
                    midi_io = io.BytesIO(file_data[idx])
                    midi = mido.MidiFile(file=midi_io)
                    if midi.type == 1:
                        merged_tracks = mido.merge_tracks(midi.tracks)
                        new_midi = mido.MidiFile(type=0)
                        new_midi.ticks_per_beat = midi.ticks_per_beat
                        new_midi.tracks.append(merged_tracks)
                        temp_list = list(file_table[idx])
                        #temp_list[0] = file_table[idx][0][:-4] + "_type0.mid"
                        file_table[idx] = tuple(temp_list)
                        output_io = io.BytesIO()
                        new_midi.save(file=output_io)
                        file_data[idx] = output_io.getvalue()
                        # update file size
                        temp_list = list(file_table[idx])
                        temp_list[1] = len(file_data[idx])
                        file_table[idx] = tuple(temp_list)
                        print("New Midi lenght ", len(file_data[idx]))
                        if 0:
                            filename = file_table[idx][0][:-4] + "_type0.mid"
                            print("saving file: " + filename)
                            with open(filename, 'wb') as output_file:
                                output_file.write(file_data[idx])

        # Save the converted files to disk
        create_grp_file(output_path=new_file_path, file_table=file_table, file_data=file_data, use_file_list = downgrade)
        
def main():
    parser = argparse.ArgumentParser(description="Convert GRP files.")
    parser.add_argument("input_path", nargs='?', default="DUKE3D.GRP", help="Path to the input GRP file.")
    parser.add_argument("output_path", nargs='?', default="DUKE3D_CONV.GRP", help="Path to the output GRP file.")
    parser.add_argument("--downgrade", action="store_true", help="Use the file list from duke3d_file_list.py, to downgrade from Atomic Edition to original DUKE3D.GRP.")
    args = parser.parse_args()

    convert_grp_file(file_path = args.input_path, new_file_path = args.output_path, downgrade = args.downgrade)

if __name__ == '__main__':
    main()