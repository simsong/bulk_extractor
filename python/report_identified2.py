#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database
#
#
# In the initial implementaiton, if a block appears in more than
# one target file or at more than one place in a target file, it's a wildcard

import os,pickle
import collections


BLOCK_SIZE = 4096
SECTOR_SIZE = 512

class Block:
    def __init__(self,disk_offset=None):
        self.disk_offset = disk_offset
        self.disk_sector = disk_offset / SECTOR_SIZE
        self.disk_sector_mod8 = self.disk_sector % 8
        self.target_filenames_and_offsets = defaultdict(set) # 
        self.allocated_file = None
        self.md5 = None
        self.flags = None
        self.count = None
    def __repr__(self):
        return str("[{}|{}]".format(self.disk_offset,self.file_offset))
        
class Blockdb:
    def __init__(self):
        self.disk_offsets = dict()                      # each offset uniquely defines one block
        self.block_hashes = collections.defaultdict(set) # set of blocks with the same hash
        self.allocated_files = collections.defaultdict(set) # each allocated file matches a set of blocks
        self.target_filenames = collections.defaultdict(set)
        self.target_file_offsets = collections.defaultdict(set)
        self.counts = collections.defaultdict(set)
    def get(self,disk_offset):
        try:
            return self.disk_offsets[disk_offset]
        except KeyError:
            block = Block(disk_offset=disk_offset)
            self.disk_offsets[disk_offset] = block
            return block
    def set_md5(self,block,sector_hash):
        block.md5 = sector_hash
        self.block_hashes[sector_hash].add(block)
    def set_target_filename_and_offset(self,block,target_filename,target_file_offset):
        block.target_filename_and_offsets[target_filename].add(target_file_offset)
        self.target_filenames[target_filename].add(block)
    def set_target_file_offset(self,block,target_file_offset):
        block.target_file_offset = target_file_offset
        block.target_file_block  = target_file_offset / BLOCK_SIZE
        self.target_file_offsets[target_file_offset].add(block)
    def set_count(self,block,count):
        block.count = count
        self.counts[count].add(block)
    def score_for_blocks(self,blocks):
        return sum([1/block.count for block in blocks])
    def blocks_for_target_filename(self,target_filename):
        return self.target_filenames[target_filename]
    def count_distinct_blocks(self,blocks):
        return len(filter(lambda block:block.count==1,blocks))


        
blockdb = Blockdb()

def make_runs(a):
    a = sorted(list(a))
    ret = []
    first = None
    while len(a):
       if not first:
           first = a.pop(0)
           last  = first
           if not a:
               ret.append((first,last))
           continue
       if last+1 == a[0]:
           last = a.pop(0)
           if not a:
               ret.append((first,last))
           continue
       ret.append((first,last))
       first = None
    return ret

#
# New code goes here
#
def read_identified_blocks(fn):
    for line in open(fn):
        if line[0]=='#': continue
        try:
            (disk_offset,sector_hash,count) = line.split("\t")
            disk_offset = int(disk_offset)
        except ValueError:
            continue
        block = blockdb.get(disk_offset)
        flags = ''
        if 'H' in count:
            count = count.replace('H','')
            flags += "H"
        if 'W' in count:
            count = count.replace('W','')
            flags += "W"
        if 'R' in count:
            count = count.replace('R','')
            flags += "R"
        count = count.replace(' ','')
        count = int(count)
        blockdb.set_md5(block,sector_hash)
        blockdb.set_count(block,count)

def read_identified_blocks_expanded(fn):
    for line in open(fn):
        if line[0]=='#': continue
        (disk_offset,sector_hash,meta) = line.split("\t")
        disk_offset = int(disk_offset)
        block = blockdb.get(disk_offset)
        target_filename    = meta.split(",")[1].split('=')[1]
        target_file_offset = int(meta.split(",")[2].split('=')[1])
        blockdb.set_target_filename_and_offset(block,target_filename,target_file_offset)


def clean_target_filename(target_filename):
    if sys.version_info >= (3,0,0):
        target_filename=target_filename.replace(u'\U0010001c',"")
    else:
        target_filename=target_filename.replace(b"\xe2\xa0\xc3\xc3\xa3","")
    return target_filename

def compute_target_filesize(target_filename):
    # Given a target filename, try to figure out its size
    target_filename = clean_target_filename(target_filename)
    if os.path.exists(target_filename): return os.path.getsize(target_filename)
    return 0

def report_target_filename(target_filename):
    blocks = blocksdb.blocks_for_target_filename(target_filename)

    # Get the runs
    target_file_runs = make_runs([block.target_


def report(outfile):
    #
    # process the files in order.  
    # Each report output line is a tuple that has score 
    # 
    
    if sys.version_info >= (3,0,0):
        of = open(outfile, 'w', newline='')
    else:
        of = open(outfile, 'wb')
    import csv
    ofwriter = csv.writer(of,  dialect='excel')

    # Get the list of all the target_filenames present and order them
    scores = []
    for target_filename in blockdb.target_filenames:
        blocks = blockdb.blocks_for_target_filename(target_filename)
        if blockdb.count_distinct_blocks(blocks)==0: continue # only those with distinct blocks
        score  = blockdb.score_for_blocks(blocks)
        scores += [(score,target_filename)]
    
    # For each target filename, generate a report line
    for (score,target_filename) in sorted(scores):
        print("filename: {}   score: {}".format(target_filename,score))
        rep = report_target_filename(target_filename)
    return

    # Inverse Sort by aligned blocks
    filenames_sorted = sorted(file_offsets,key=lambda a:aligned_blocks_per_file[a][0],reverse=True)
    first = True
    for filename in filenames_sorted:
        b1  = min(file_blocks[filename]) # first block seen
        b2  = max(file_blocks[filename]) # last block seen
        num = len(file_blocks[filename]) # number of blocks seen
        missing = (b2-b1+1)-num          # missing between b1 and b2

        # Tabulate the statistics on this file
        # counts keeps track of the number of 'distinct' counts for each hash in the file
        distinct_count = 0
        distinct_count_uncommon = 0
        total_count_uncommon = 0
        hcount = 0
        wcount = 0
        rcount = 0
        count  = 0
        
        for hash in file_sector_hashes[filename]:
            count += 1
            common = (hash in h_hashes) or (hash in w_hashes) or (hash in r_hashes)
            if not common:
                total_count_uncommon += 1
            if hash_counts[hash]==1:
                distinct_count += 1
                if not common:
                    distinct_count_uncommon += 1
                    
        if distinct_count==0:
            continue

        runs = make_runs(file_blocks[filename])
        lens = [a[1]-a[0]+1 for a in runs]
        longest_run    = max(lens)

        distinct_runs = make_runs(distinct_file_blocks[filename])
        distinct_lens = [a[1]-a[0]+1 for a in distinct_runs]
        longest_distinct_run = max(distinct_lens)
        
        pmatch         = filename
        blocks_seen    = count
        aligned_blocks = aligned_blocks_per_file[filename][0]
        aligned_percent= aligned_blocks_per_file[filename][1]*100.0
        distinct_count = distinct_count
        distinct_percent = distinct_count/count * 100.0
        block_range      = "{}--{}".format(b1,b2)
        
        if first:
            ofwriter.writerow(['Match','Blocks Seen','Range',
                               'Longest Run',
                               'Longest Distinct Run',
                               'Blocks Aligned','% Aligned',
                               'BLocks Distinct','% distinct'])
            first = False
        ofwriter.writerow([pmatch,blocks_seen,block_range,
                           longest_run,
                           longest_distinct_run,
                           aligned_blocks,aligned_percent,distinct_count,distinct_percent])

if __name__=="__main__":
    import sys
    import argparse
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--notaligned",help="show matches that are not aligned",action='store_true')
    parser.add_argument("reportdir",help="Report directory")
    parser.add_argument("--save",help='saves database in file')
    parser.add_argument("--load",help='saves database in file')
    args = parser.parse_args()

    if args.reportdir.endswith("/") or args.reportdir.endswith("\\"):
        args.reportdir = args.reportdir[:-1]
    report_output = os.path.join(args.reportdir,"blocks-report.csv")

    if not os.path.exists(args.reportdir):
        print("{} does not exist".format(args.reportdir))

    if args.load:
        with open(args.load,'rb') as f:
            blockdb = pickle.load(f)

    else:
        identified_fn = os.path.join(args.reportdir,"identified_blocks.txt")
        if os.path.exists(identified_fn):
            read_identified_blocks(identified_fn)

        expanded_fn = os.path.join(args.reportdir,"identified_blocks_expanded.txt")
        if os.path.exists(expanded_fn):
            read_identified_blocks_expanded(expanded_fn)

    if args.save:
        with open(args.save,'wb') as f:
            pickle.dump(blockdb,f)

    report(report_output)
    print("Output report in {}".format(report_output))
