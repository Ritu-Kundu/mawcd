import string
import random
import math
import os
import subprocess
import time
import sys
import codecs
import collections

# #################################################
# Script for the preliminary run of the experiments.
# #################################################


######### Settings of the experiments #############
DATA_FOLDER = './data/'
EXPERIMENT_FOLDER = './result/'
TOOL = './bin/mawcd'
DATA_EXTENSION = '.fa.gz'
CURED_EXTENSION = '.cured'

ALPHABET = ['A', 'C', 'G', 'T', 'N']

param_separator = '\t'
###################################################
INDEX_FILE_NAME = './scripts/quick.index'
MERGED_FILE_NAME = EXPERIMENT_FOLDER + 'sequence.merged'
AD_FILE_NAME = EXPERIMENT_FOLDER + 'sequence.merged.ad'

STATS_FILE_NAME = EXPERIMENT_FOLDER + 'expertiments.stats'
AD_STATS_FILE_NAME = EXPERIMENT_FOLDER + 'ad.stats'
LOG_FILE_NAME = EXPERIMENT_FOLDER + 'experiments.log'

FILES = []


stats_param = ['file_name', 'comp_time', 'decom_time', 'comp_size',  'gz_size']


def memory_usage_resource():
    import resource
    rusage_denom = 1024.
    if sys.platform == 'darwin':
        # OSX produces the output in different units
        rusage_denom = rusage_denom * rusage_denom
    mem = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / rusage_denom
    return mem


def hms_string(sec_elapsed):
    h = int(sec_elapsed / (60 * 60))
    m = int((sec_elapsed % (60 * 60)) / 60)
    s = sec_elapsed % 60.
    return "{}:{:>02}:{:>05.2f}".format(h, m, s)
# End hms_string


def cure_char(c):
    if c.upper() in ALPHABET:
        return c.upper()
    else:
        print ('******* INVALID CHAR *******' + c)
        return 'A'

# Cure the files


def cure_files(log_file):
    # Cretae index file
    cmd = 'ls -1 ' +  DATA_FOLDER + ' > ' + INDEX_FILE_NAME
    comp = subprocess.Popen(cmd, shell=True)
    comp.wait()
    print ('Index created : ' + INDEX_FILE_NAME + '\n')

    # Open a file to write merged sequence
    merged_f = open(MERGED_FILE_NAME, 'w')

    # Extract each file, create cured copy, delete original, merge sequences
    with open(INDEX_FILE_NAME) as index_f:
        f_ind = 0
        for index_line in index_f:
            gz_name = index_line.strip()
            f_ind = f_ind + 1

            # exrtract file
            cmd = 'gunzip -k ' +  DATA_FOLDER + gz_name
            comp = subprocess.Popen(cmd, shell=True)
            comp.wait()
            print ('Extracted file num : ' + str(f_ind) + '\n')

            # open file to write its cured version
            ext = len('.gz')
            f_name = gz_name[0:-ext]
            cured_f = open(EXPERIMENT_FOLDER + f_name + CURED_EXTENSION, 'w')
            FILES.append(f_name)

            # start reading and writing sequences
            with open(DATA_FOLDER + f_name) as f:
                for line in f:
                    if line.strip() != '' and line[0] != '>':
                        # write in file if valid
                        cured_line = ''.join(cure_char(c)
                                             for c in line.strip())
                        cured_f.write(cured_line)
                        merged_f.write(cured_line)
                f.close()
                # delete this file
                cmd = 'rm ' + DATA_FOLDER +f_name
                comp = subprocess.Popen(cmd, shell=True)
                comp.wait()

        index_f.close()
        log_file.write("Number of files: " + str(len(FILES)) + '\n')

# extract key_size and number of total keys from the anti-dictionary


def collect_ad_stats(ad_file):
    key_size = 0
    num_keys = 0
    with open(ad_file, "rb") as f:

        # read key-size
        chunk = f.read(1)
        if chunk:
            key_size = int.from_bytes(
                  chunk, byteorder='little', signed=False)

            # read num keys of size 1B in ad0
            num_keys0_1 = int.from_bytes(
                    f.read(1), byteorder='little', signed=False)
            # read num keys of size 1B in ad1
            num_keys1_1 = int.from_bytes(
                    f.read(1), byteorder='little', signed=False)
            # read num keys of size 2B in ad0
            num_keys0_2 = int.from_bytes(
                    f.read(2), byteorder='little', signed=False)
            # read num keys of size 2B in ad1
            num_keys1_2 = int.from_bytes(
                    f.read(2), byteorder='little', signed=False)
            # read num keys of size 4B in ad0
            num_keys0_4 = int.from_bytes(
                    f.read(4), byteorder='little', signed=False)
            # read num keys of size 4B in ad1
            num_keys1_4 = int.from_bytes(
                    f.read(4), byteorder='little', signed=False)
            num_keys = num_keys0_1 + num_keys1_1 + num_keys0_2 + \
                    num_keys1_2 + num_keys0_4 + num_keys1_4
        f.close()
    return (key_size, num_keys)

# Create anti-dictionary


def create_ad(log_file):
    # Append reverse compliment in the reference
    f = open(AD_STATS_FILE_NAME, 'w')

    ad_cmd = TOOL + ' -m AD -a DNA -i ' + MERGED_FILE_NAME + ' -d ' + AD_FILE_NAME
    print (ad_cmd)
    t = time.time()
    comp = subprocess.Popen(ad_cmd, stdout=subprocess.PIPE, shell=True)
    comp.wait()
    ad_time = hms_string(time.time() - t)
    print("Anti-dictionary created: " + AD_FILE_NAME + '\n')

    (key_size, num_keys) = collect_ad_stats(AD_FILE_NAME)
    ad_size = os.path.getsize(AD_FILE_NAME)

    log_file.write("Anti-dictionary created: key-size: num-keys: " +
                   str(key_size) + ' : ' + str(num_keys) + '\n')

    # write stats file
    f.write('# AD size :' + str(ad_size) + '\n')
    f.write('# AD creation time :' + ad_time + '\n')
    f.write('# AD key-size :' + str(key_size) + '\n')
    f.write('# AD num-keys :' + str(num_keys) + '\n')
    f.close()


def compare_files(orig_file_name, cd_file_name, log_file):
    with open(orig_file_name) as f1, open(cd_file_name) as f2:
        for line1, line2 in zip(f1, f2):
            if line1 != line2:
                l1 = len(line1)
                l2 = len(line2)
                log_file.write('Difference in files: ' +orig_file_name + ' orig_len : comp_len ' + str(l1) + ' : ' + str(l2))
                print ('******* INVALID COMPRESSION *******')
                f1.close()
                f2.close()
                return False
            f1.close()
            f2.close()
            return True   


def main():
    lf = open(LOG_FILE_NAME, 'w')
    sf = open(STATS_FILE_NAME, 'w')
    sf.write(param_separator.join(stats_param))
    sf.write('\n')

    # ############# Cure files #############
    cure_files(lf)
    total_files = len(FILES)
    # ############# Create Anti-dictionary #############
    #create_ad(lf)

    ind = 0
    while ind < total_files:
        f_name = FILES[ind]
        lf.write('\n $$$$$$$$c' + f_name + 'c$$$$$$$$ \n')
        print('$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$  ' +
              f_name + '  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$')
        
        # ############# Compress #############
        cured_fname = EXPERIMENT_FOLDER + f_name + CURED_EXTENSION
        com_cmd = TOOL + ' -m COM -a DNA -i ' + cured_fname + ' -d ' + AD_FILE_NAME
        print (com_cmd)
        t = time.time()
        comp = subprocess.Popen(com_cmd, stdout=subprocess.PIPE, shell=True)
        comp.wait()
        com_time = hms_string(time.time() - t)
        print("File compressed: \n")
        lf.write("File compressed: \n")
        com_size = os.path.getsize(cured_fname)

        # ############# Decompress #############
        comp_fname = EXPERIMENT_FOLDER + f_name + CURED_EXTENSION + '.com'
        decom_cmd = TOOL + ' -m DECOM -a DNA -i ' + comp_fname + ' -d ' + AD_FILE_NAME
        print (decom_cmd)
        t = time.time()
        comp = subprocess.Popen(decom_cmd, stdout=subprocess.PIPE, shell=True)
        comp.wait()
        decom_time = hms_string(time.time() - t)
        print("File decompressed: \n")
        lf.write("File decompressed: \n")

        # ############# Verify #############
        if compare_files(cured_fname, comp_fname+'.decom', lf):
           print("File Compression Valid. \n")
           lf.write("File Compression Valid. \n")
        else:
            print("COMPRESSION WRONG!!!!!!\n")
            lf.write("COMPRESSION WRONG!!!!!!\n \n")
            break

        # ############# Compress with gz #############
        cured_fname = EXPERIMENT_FOLDER + f_name + CURED_EXTENSION
        gz_cmd = 'gzip -k ' + cured_fname
        print (gz_cmd)
        # t = time.time()
        comp = subprocess.Popen(gz_cmd, stdout=subprocess.PIPE, shell=True)
        comp.wait()
        # com_time = hms_string(time.time() - t)
        print("File compressed using gzip: \n")
        lf.write("File compressed using gzip: \n")
        gz_size = os.path.getsize(cured_fname+'.gz')
        # remove gz file
        comp = subprocess.Popen('rm ' + cured_fname+'.gz', stdout=subprocess.PIPE, shell=True)

        # ############# Write stats #############
        sf.write(f_name + param_separator)
        sf.write(com_time + param_separator)
        sf.write(decom_time + param_separator)
        sf.write(str(com_size) + param_separator)
        sf.write(str(gz_size) + '\n')
        
        # read next 
        ind =ind +1
    
    
    lf.close()
    sf.close()
    print('======================== Success!!!!============================')


main()
