#!/usr/bin/env python3
# NAME: Megan Pham, Cody Do
# EMAIL: megspham@ucla.edu, D.Codyx@gmail.com
# ID: 505313300, 105140467

import csv
import sys

##########################
###  GLOBAL VARIABLES  ###
##########################

inodes=dict()
allBlocks = dict() 
allocatedInodes = set() 
inodebitmap=[]
blockbitmap=[]
group = None
sb= None
dirents=[]
errors = 0

#################
###  CLASSES  ###
#################

class inode():
    def __init__(self, frow):
        self.inum = int(frow[1])
        self.group = int(frow[5])
        self.link = int(frow[6])
        self.direct= []
        self.single=0
        self.double =0
        self.triple= 0
        
class superblock():
    def __init__(self, frow):
        self.blocknum = int(frow[1])
        self.inodenum = int(frow[2])
        self.blocksz = int(frow[3])
        self.inodesz = int(frow[4])
        self.nonreserved = int(frow[7])

class Group():
    def __init__(self, frow):
        self.blocknum = int(frow[2])  
        self.inodenum = int(frow[3])
        self.inodesblock = int(frow[8])


#this class for both 
class blockData():
    def __init__(self,blocknum,inum, offset, level):
        self.blocknum = blocknum
        self.inum = inum
        self.offset = offset
        self.level = level

class directory():
    def __init__(self, frow):
        self.pnum = int(frow[1])
        self.inum = int(frow[3])
        self.name = frow[6]

###################
###  FUNCTIONS  ###
###################

def handleInodeAudit(sb, inodeMap):
    for i in inodes:
        allocatedInodes.add(i) 
    for i in range(sb.nonreserved, sb.inodenum):
        if i not in allocatedInodes and i not in inodeMap:
            print("UNALLOCATED INODE " + str(i)+ " NOT ON FREELIST")
    for i in allocatedInodes:
        if i in inodeMap:
            print("ALLOCATED INODE " +str(i)+ " ON FREELIST")

def handleBlockAudit(sb, group):
    global errors
    highestblock = sb.blocknum
    start = int(group.inodesblock + (sb.inodesz * group.inodenum)/sb.blocksz)
    
    #check for unreferenced blocks
    for i in range(start,highestblock):
        if i not in blockbitmap and i not in allBlocks:
            print("UNREFERENCED BLOCK " + str(i))
            errors +=1
            
    for j in allBlocks:
        tempBlock = list(allBlocks[j])
        makeBlock = tempBlock[0]
        
        numBlock = makeBlock.blocknum
        inode = makeBlock.inum
        offset = makeBlock.offset
        levelBlock = makeBlock.level
        
        if levelBlock == 1:
            typeBlock = " INDIRECT "
        elif levelBlock == 2:
            typeBlock = " DOUBLE INDIRECT "
        elif levelBlock == 3:
            typeBlock = " TRIPLE INDIRECT "
        else:
            typeBlock = " "

        if j < 0 or j >= highestblock:
            errors +=1
            print("INVALID" + str(typeBlock)+ "BLOCK " + str(numBlock) +
            " IN INODE " + str(inode) + " AT OFFSET " + str(offset))
        elif numBlock in blockbitmap: 
            errors +=1
            print ("ALLOCATED BLOCK " + str(numBlock) + " ON FREELIST")
        elif j < start:
            errors +=1
            print("RESERVED" + str(typeBlock)+ "BLOCK " + str(numBlock) +
            " IN INODE " + str(inode) + " AT OFFSET " + str(offset))
        elif len(tempBlock) > 1:
            for dup in tempBlock:
                level = dup.level
                dupNumBlock = dup.blocknum
                dupInode = dup.inum
                dupOff = dup.offset
                errors +=1
                if level == 1:
                    print("DUPLICATE INDIRECT BLOCK "+ str(dupNumBlock)+ " IN INODE " +str(dupInode) + " AT OFFSET " + str(dupOff))
                elif level == 2:
                    print("DUPLICATE DOUBLE INDIRECT BLOCK "+ str(dupNumBlock)+ " IN INODE " +str(dupInode) + " AT OFFSET " + str(dupOff))
                elif level == 3:
                    print("DUPLICATE TRIPLE INDIRECT BLOCK "+ str(dupNumBlock)+ " IN INODE " +str(dupInode) + " AT OFFSET " + str(dupOff))
                else:
                    print("DUPLICATE BLOCK "+ str(dupNumBlock)+ " IN INODE " +str(dupInode) + " AT OFFSET " + str(dupOff))

def handleDirectoryAudit(inodeList, superBlock, directory, inodeMap):
    # numLinks = link count of inodes existing in directories 
    numLinks = [0] * superBlock.inodenum
    parentList = [0] * superBlock.inodenum
    parentList[2] = 2 

    # Scan content and check if inode number is valid. If not, output
    # the proper message
    for d in directory:
        # d is not allocated and exists in the the inode bitmap as free
        if d.inum not in allocatedInodes and d.inum in inodeMap: #not allocated and in the inode bitmap as free
            print("DIRECTORY INODE " + str(d.pnum) + " NAME " + str(d.name) + " UNALLOCATED INODE " + str(d.inum))
        elif d.inum > superBlock.inodenum or d.inum <1:
            print("DIRECTORY INODE " + str(d.pnum) + " NAME " + str(d.name) + " INVALID INODE " + str(d.inum))
        else:
            # enumerate the link
            numLinks[d.inum] += 1 

    # Scan content and save pointer to parent
    for d in directory:
            if d.name != "'.'" and d.name != "'..'":
                parentList[d.inum] = d.pnum

    # Scan inodes and check for mismatching link counts
    for inode in inodeList.values():
        if inode.link != numLinks[inode.inum]:
            print("INODE " + str(inode.inum) + " HAS " + str(numLinks[inode.inum]) + " LINKS BUT LINKCOUNT IS " + str(inode.link))
    
    # Scan to check for inconsistencies of . and ..
    for  d in directory:
        if d.name == "'.'" and d.inum !=d.pnum:
            print("DIRECTORY INODE " + str(d.pnum) + " NAME '.' LINK TO INODE " + str(d.inum) + " SHOULD BE " + str(d.pnum))
        elif d.name =="'..'" and d.inum != parentList[d.inum]:
            print("DIRECTORY INODE " + str(d.pnum) + " NAME '..' LINK TO INODE " + str(d.inum) + " SHOULD BE " + str(parentList[d.inum]))

def main():
    # Check number of arguments
    if len(sys.argv) < 2:
        sys.stderr.write("Error: Insufficient number of arguments\n")
        sys.exit(1)

    # Attempt to open csv file
    try:
        inputCSV = open(sys.argv[1], 'r')
    except IOError:
        sys.stderr.write("Error: Unable to open csv file\n")
        sys.exit(1)

    readCSV = csv.reader(inputCSV)

    # Loop through each row in the read csv file
    for row in readCSV:
        checkMe = row[0]
        if checkMe == 'SUPERBLOCK':
            sb = superblock(row)
        elif checkMe == 'GROUP':
            group = Group(row)
        elif checkMe == 'BFREE':
            blockbitmap.append(int(row[1]))
        elif checkMe == 'IFREE':
            inodebitmap.append(int(row[1])) 
        elif checkMe == 'INODE':
            # Create a single inode and place into dictionary of inodes
            # Key: inode number, Value: inode itself
            singleInode = inode(row)
            singleInode.direct = [int(i) for i in row[12:-3]]
            singleInode.single =int(row[-3])
            singleInode.double =int(row[-2])
            singleInode.triple= int(row[-1])
            inodes[int(row[1])] = singleInode

            for i in range(15):
                address = int(row[12+i])

                # Create newBlock in first 12 of 15
                if i >= 12 and address > 0:
                    level = i - 11
                    offset = i

                    if level == 1:
                        offset = 12
                    elif level == 2:
                        offset = 268
                    elif level == 3:
                        offset = 65804

                    newBlock = blockData(address, int(row[1]), offset, level)

                # Create newBlcok in last 3 of 15
                elif address > 0:
                    newBlock = blockData(address, int(row[1]), i, 0 )

                # React accordingly with the created newBlock (from above)
                if address > 0:
                    if address not in allBlocks:
                        allBlocks[address] = set()
                    allBlocks[address].add(newBlock)
                
        elif checkMe =='DIRENT':
            dirents.append(directory(row))

        elif checkMe =='INDIRECT':
            newBlock = blockData(int(row[5]), int(row[1]), int(row[3]),int(row[2]) - 1 )
            if int(row[5]) not in allBlocks:
                allBlocks[int(row[5])] = set()
            allBlocks[int(row[5])].add(newBlock)
                
    handleBlockAudit(sb,group)
    handleInodeAudit(sb,inodebitmap)
    handleDirectoryAudit(inodes,sb, dirents,inodebitmap)

    if errors == 0:
        sys.exit(0)
    else:
        sys.exit(2)


if __name__ =='__main__':
    main()

