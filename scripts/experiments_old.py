import string
import random
import math
import os
import subprocess
import time
import sys
######### Settings of the experiments #############
#alphabet = ['A','C', 'G', 'T']
alphabet = ['A','C']
#alphabet = ['A','C','D','E','F','G','H','I','K','L','M','N','P','Q','R','S','T','U','V','W','Y']
num_files = 100
min_seq_size = 100
max_seq_size = 100000
num_seq_in_file = 1
param_separator = '\t'
###################################################

FOLDER = './experiments/'
DATA_FOLDER = 'data/'
INPUT_FILE_NAME = 'cured'
COM_FILE_NAME = 'com'
DECOM_FILE_NAME = 'decom'
AD_FILE_NAME = 'ad'
STATS_FILE_NAME = 'stats.txt'
ERROR_FILE_NAME = 'errors.log'
DATASETS = ['proteins.001.1']
#DATASETS = ['test']


stats_param = ['file_name', 'min_seq','max_seq', 'o_file_size', 'c_file_size', 
'ad_file_size', 'com_time', 'decom_time', 'min_ad_size', 'max_ad_size', 
'min_key_size', 'max_key_size', 'min_key', 'max_key']

def memory_usage_resource():
    import resource
    rusage_denom = 1024.
    if sys.platform == 'darwin':
        # OSX produces the output in different units
        rusage_denom = rusage_denom * rusage_denom
    mem = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / rusage_denom
    return mem


def compare_files(o_file_name, cd_file_name, er_file):
    print ('comparing files: ' + o_file_name + ' and ' + cd_file_name + '\n' )
    with open(o_file_name) as f1, open(cd_file_name) as f2:
        for line1, line2 in zip(f1, f2):
            if line1 != line2:
                l1 = len(line1)
                l2 = len(line2)
                print (l1)
                print(l2)
                for i in range(min(l1, l2)):
                    if line1[i] != line2[i]:
                        print(i)
                        print(line1[1], line2[i])
                        break
                print (line1[l1-10:l1])
                print (line2[l2-10:l2])
                er_file.write('Difference in files: ' +o_file_name + ' ' + cd_file_name)
                f1.close()
                f2.close()
                return False
            f1.close()
            f2.close()
            return True

def compare_files1(o_file_name, cd_file_name, er_file):
    print ('comparing files: ' + o_file_name + ' and ' + cd_file_name + '\n' )
    with open(o_file_name) as f1, open(cd_file_name) as f2:
        text1Lines = f1.readlines()
        text2Lines = f2.readlines()
        f1.close()
        f2.close()
        set1 = set(text1Lines)
        set2 = set(text2Lines)
        diffList = (set1|set2)-(set1&set2)
        #print (diffList)
        if len(diffList) != 0:
            return False
        else:            
            return True

def collect_ad_stats(ad_file):
    key_size = []
    ad_size = []
    min_key = sys.maxsize
    max_key = 0
    with open(ad_file, "rb") as f:

        while True:
             # read sequence size and ignore it
            chunk = f.read(8)
            if chunk:
                keys = []
                # read key-size
                chunk = f.read(1)
                key_size.append(int.from_bytes(chunk, byteorder='little', signed=False))
                
                # read num keys of size 1B in ad0
                num_keys0_1 = int.from_bytes(f.read(1), byteorder='little', signed=False)
                # read num keys of size 1B in ad0
                num_keys1_1 = int.from_bytes(f.read(1), byteorder='little', signed=False)
                # read num keys of size 1B in ad0
                num_keys0_2 = int.from_bytes(f.read(2), byteorder='little', signed=False)
                # read num keys of size 1B in ad0
                num_keys1_2 = int.from_bytes(f.read(2), byteorder='little', signed=False)
                # read num keys of size 1B in ad0
                num_keys0_4 = int.from_bytes(f.read(4), byteorder='little', signed=False)
                # read num keys of size 1B in ad0
                num_keys1_4 = int.from_bytes(f.read(4), byteorder='little', signed=False)
                total_size = num_keys0_1 + num_keys1_1+num_keys0_2+num_keys1_2+num_keys0_4+num_keys1_4
                ad_size.append(total_size)
                num_keys = [(num_keys0_1, 1), (num_keys0_2, 2), (num_keys0_4, 4), (num_keys1_1, 1), (num_keys1_2, 2), (num_keys1_4, 4)]
                for n, v in num_keys:
                    for i in range(n):
                        chunk = f.read(v)
                        keys.append(int.from_bytes(chunk, byteorder='little', signed=False))
                min_key = min(keys + [min_key])
                max_key = max(keys + [max_key])
            else:
                break
            
        f.close()
    min_ad_size = min(ad_size)
    max_ad_size = max(ad_size)
    min_key_size = min(key_size)
    max_key_size = max(key_size)
    return [min_ad_size, max_ad_size, min_key_size, max_key_size, min_key, max_key]                



def write_file(seq_filename):
    seq_file = open(seq_filename,'w')
    seq_sizes = []
    for i in range(num_seq_in_file):
        seq_len = random.randint(min_seq_size, max_seq_size)
        seq_sizes.append(seq_len)
        seq_file.write('>seq ' + str(i) + '\n')
        txt = ''.join(random.choice(alphabet) for i in range(seq_len))
        seq_file.write(txt+'\n\n')
    seq_file.close()
    return [min(seq_sizes), max(seq_sizes)]

def cure_char2(c):
    if c.upper() in alphabet:
        return c.upper()
    else:
        return random.choice(alphabet)

def cure_file2(ip_filename, op_filename):
    seq_sizes = []
    with open(ip_filename, 'r') as f1, open(op_filename, 'w') as f2:
        for line1 in f1:
            if line1.strip() != '':
                line2 = ''.join(cure_char(c) for c in line1.strip()) + '\n'
                seq_sizes.append(len(line2))
            else:
                line2 = line1
            f2.write(line2)            
        f1.close()
        f2.close()
    return [min(seq_sizes), max(seq_sizes)]


def cure_char1(c):
    if c in alphabet:
        if c=='0':
            return 'A'
        else:
            return 'C'
    else:
        print("INVALID CHAR: " + c)
        return random.choice(alphabet)

def cure_file1(ip_filename, op_filename):
    seq_sizes = []
    with open(ip_filename, 'r') as f1, open(op_filename, 'w') as f2:
        for line1 in f1:
            if line1.strip() != '':
                line2 = ''.join(cure_char(c) for c in line1.strip()) + '\n'
                seq_sizes.append(len(line2))
            else:
                line2 = line1
            f2.write(line2)            
        f1.close()
        f2.close()
    return [min(seq_sizes), max(seq_sizes)]

def cure_file(ip_filename, op_filename):
    seq_sizes = []
    with open(ip_filename, 'r') as f1, open(op_filename, 'w') as f2:
        for line in f1:
            if line.strip() != '':
                for c in line.strip():
                    if c in alphabet:
                        f2.write(c) 
                    else:
                        print("INVALID CHAR: " + c)
                        f2.write(random.choice(alphabet))
                seq_sizes.append(len(line))
                f2.write("\n")
            else:
                line2 = line
                f2.write(line2)            
        f1.close()
        f2.close()
    return [min(seq_sizes), max(seq_sizes)]

def cure_file4(ip_filename, op_filename):
    seq_sizes = []
    with open(ip_filename, 'r') as f1, open(op_filename, 'w') as f2:
        for line1 in f1:
            if line1.strip() != '':
                seq_sizes.append(len(line1))
            f2.write(line1)            
        f1.close()
        f2.close()
    return [min(seq_sizes), max(seq_sizes)]


def main():
    i_filename = FOLDER + DATA_FOLDER + INPUT_FILE_NAME
    c_filename = FOLDER + DATA_FOLDER + COM_FILE_NAME
    d_filename = FOLDER + DATA_FOLDER + DECOM_FILE_NAME
    ad_filename = FOLDER + DATA_FOLDER + AD_FILE_NAME
    ef = open(FOLDER + ERROR_FILE_NAME, 'w')
    sf = open(FOLDER + STATS_FILE_NAME,'w')
    sf.write(param_separator.join(stats_param))
    sf.write('\n')

    for f in DATASETS:
        print('$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$  ' + f + '  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4')
        suff =  '.'+f
        # Generate file
        #min_seq_len, max_seq_len = cure_file(FOLDER + DATA_FOLDER + f, i_filename + suff)
        min_seq_len, max_seq_len = [1,1]

        # Call the tool to compress
        tool = './bin/mawcd'
        com_cmd = tool + ' -m COM -a PROT -i ' + i_filename + suff + ' -o ' + c_filename + suff + ' -d ' + ad_filename + suff
        print('COMPRESSING: ' + com_cmd)
        t = time.process_time()
        #comp = subprocess.Popen(com_cmd, shell=True)
        #comp.wait()
        c_time = time.process_time() - t

        # Call the tool to decompress
        decom_cmd = tool + ' -m DECOM -a PROT -i ' + c_filename + suff + ' -o ' + d_filename + suff + ' -d ' + ad_filename + suff
        print('DECOMPRESSING: ' + decom_cmd)
        t = time.process_time()
        decomp = subprocess.Popen(decom_cmd, shell=True)
        decomp.wait()
        d_time = time.process_time() - t

        # Write stats
        o_file_size = os.path.getsize(i_filename + suff)
        c_file_size = os.path.getsize(c_filename + suff)
        ad_file_size = os.path.getsize(ad_filename + suff)
        sf.write(INPUT_FILE_NAME + suff+param_separator)
        sf.write(str(min_seq_len) + param_separator + str(max_seq_len) + param_separator)
        sf.write(str(o_file_size) + param_separator + str(c_file_size) + param_separator + str(ad_file_size) + param_separator)
        sf.write(str(c_time*1000) + param_separator + str(d_time*1000) + param_separator)
        ad_stats = collect_ad_stats(ad_filename+suff)
        sf.write(param_separator.join(map(str,ad_stats)))
        sf.write('\n')

        # Check correctness
        if not compare_files(i_filename+suff, d_filename+suff, ef):
            print('ERROR: ')
            break
    ef.close()
    sf.close()
    print('======================== Success!!!!============================')


def main_random():
    i_filename = FOLDER + DATA_FOLDER + INPUT_FILE_NAME
    c_filename = FOLDER + DATA_FOLDER + COM_FILE_NAME
    d_filename = FOLDER + DATA_FOLDER + DECOM_FILE_NAME
    ad_filename = FOLDER + DATA_FOLDER + AD_FILE_NAME
    ef = open(FOLDER + ERROR_FILE_NAME, 'w')
    sf = open(FOLDER + STATS_FILE_NAME,'w')
    sf.write(param_separator.join(stats_param))
    sf.write('\n')

    for i in range(num_files):
        suff = str(i) + '.txt'
        # Generate file
        min_seq_len, max_seq_len = write_file(i_filename + suff)

        # Call the tool to compress
        tool = './bin/mawcd'
        com_cmd = tool + ' -m COM -a DNA -i ' + i_filename + suff + ' -o ' + c_filename + suff + ' -d ' + ad_filename + suff
        print('COMPRESSING: ' + com_cmd)
        t = time.process_time()
        comp = subprocess.Popen(com_cmd, shell=True)
        comp.wait()
        c_time = time.process_time() - t

        # Call the tool to decompress
        decom_cmd = tool + ' -m DECOM -a DNA -i ' + c_filename + suff + ' -o ' + d_filename + suff + ' -d ' + ad_filename + suff
        print('DECOMPRESSING: ' + decom_cmd)
        t = time.process_time()
        #decomp = subprocess.Popen(decom_cmd, shell=True)
        #decomp.wait()
        d_time = time.process_time() - t

        # Check correctness
        if not compare_files(i_filename+suff, d_filename+suff, ef):
            print('ERROR: ')
            break
        # Write stats
        o_file_size = os.path.getsize(i_filename + suff)
        c_file_size = os.path.getsize(c_filename + suff)
        ad_file_size = os.path.getsize(ad_filename + suff)
        sf.write(INPUT_FILE_NAME + suff+param_separator)
        sf.write(str(min_seq_len) + param_separator + str(max_seq_len) + param_separator)
        sf.write(str(o_file_size) + param_separator + str(c_file_size) + param_separator + str(ad_file_size) + param_separator)
        sf.write(str(c_time*1000) + param_separator + str(d_time*1000) + param_separator)
        ad_stats = collect_ad_stats(ad_filename+suff)
        sf.write(param_separator.join(map(str,ad_stats)))
        sf.write('\n')
    ef.close()
    sf.close()
    print('$$$$$$$$$$$$$$$$$Success!!!!')
            
   

main()
