import sys

block_bitmap_nums = set()  # Block numbers of the block bitmaps of all groups
inode_bitmap_nums = set()  # Block numbers of the inode bitmaps of all groups
free_blocks = set()  # Block numbers of all free blocks acc to bitmap.csv
free_inodes = set()  # Block numbers of all free inodes acc to bitmap.csv
inodes = []  # List containing all inodes as Inode objects sorted by inode number
indirect_blocks = dict()  # Map parent inode_number to list of entry_num and pointer value
directories = []  # List containing all directories as Directory objects sorted by parent inode number
inodes_per_group, total_inodes, largest_block_number = None, None, None


class Inode(object):
    def __init__(self, inode_number, blocks, links_count, num_blocks):
        self.inode_number = inode_number
        self.blocks = blocks
        self.links_count = links_count
        self.num_blocks = int(num_blocks)


class Directory(object):
    def __init__(self, parent_inode_num, entry_num, inode_num_file, name):
        self.parent_inode_num = parent_inode_num
        self.entry_num = entry_num
        self.inode_num_file = inode_num_file
        self.name = name


def traverse_indirect_reference(inode, blocks, block_mapping, cur_level, max_level):
    if cur_level == max_level or not blocks:
        return
    pass_in = []
    for block in blocks:
        if block not in indirect_blocks:
            continue
        for matching in indirect_blocks[block]:
            entry_num = matching[0]
            value = matching[1]
            if value not in block_mapping:
                block_mapping[value] = []
            block_mapping[value].append([inode.inode_number, entry_num])
            pass_in.append(value)
    traverse_indirect_reference(inode, pass_in, block_mapping, cur_level + 1, max_level)


def traverse_indirect(inode, blocks, output_mapping, cur_level, max_level):
    if cur_level == max_level or not blocks:
        return
    pass_in = []
    for block in blocks:
        if block not in indirect_blocks:
            continue
        for matching in indirect_blocks[block]:
            entry_num = matching[0]
            value = matching[1]
            if str(int(value, 16)) in free_blocks:
                if value not in output_mapping:
                    output_mapping[value] = []
                output_mapping[value].append([inode.inode_number, entry_num, block])
            pass_in.append(value)
    traverse_indirect(inode, blocks, output_mapping, cur_level + 1, max_level)


def traverse_indirect_blocks(inode, blocks, cur_level, max_level, traversed):
    if cur_level == max_level or not blocks:
        return
    pass_in = []
    for block in blocks:
        if block not in indirect_blocks:
            continue
        if traversed[0] >= inode.num_blocks:
            return
        for block_info in indirect_blocks[block]:
            entry_num = block_info[0]
            pointer_value = int(block_info[1], 16)
            if pointer_value == 0 or pointer_value > largest_block_number:
                sys.stdout.write(
                    "INVALID BLOCK < " + str(pointer_value) + " > IN INODE < " + inode.inode_number + " > " +
                    "INDIRECT BLOCK < " + str(int(block, 16)) + " > ENTRY < " + str(entry_num) + " >\n")
            else:
                pass_in.append(block_info[1])
            traversed[0] += 1
    traverse_indirect_blocks(inode, pass_in, cur_level + 1, max_level, traversed)


def unallocated_blocks():
    output_mapping = dict()  # Map block num to list of inode number,entry_num,indirect_num
    for inode in inodes:
        for index, block in enumerate(inode.blocks):
            if block == '0':
                break
            dec_val = str(int(block, 16))
            if dec_val in free_blocks:
                if block not in output_mapping:
                    output_mapping[block] = []
                output_mapping[block].append([inode.inode_number, str(index), 0])
            if index == 12:
                traverse_indirect(inode, [block], output_mapping, 0, 1)
            elif index == 13:
                traverse_indirect(inode, [block], output_mapping, 0, 2)
            elif index == 14:
                traverse_indirect(inode, [block], output_mapping, 0, 3)

    for block, values in output_mapping.items():
        output_string = ""
        dec_val = str(int(block, 16))
        sys.stdout.write("UNALLOCATED BLOCK < " + dec_val + " > REFERENCED BY ")
        for value in values:
            inode_num = value[0]
            entry_num = value[1]
            indirect_block_num = value[2]
            if indirect_block_num:
                output_string += ("INODE < " + inode_num + " > INDIRECT BLOCK < " + indirect_block_num + " > " +
                                  "ENTRY < " + entry_num + " > ")
            else:
                output_string += "INODE < " + inode_num + " > " + "ENTRY < " + entry_num + " > "
        sys.stdout.write(output_string[:-1] + "\n")


def duplicated_referenced_blocks():
    block_mapping = dict()  # Map block numbers to list of inode numbers, entry_num
    for inode in inodes:
        for index, block in enumerate(inode.blocks):
            if block == '0':
                break
            if block not in block_mapping:
                block_mapping[block] = []
            block_mapping[block].append([inode.inode_number, str(index)])
            if index == 12:
                traverse_indirect_reference(inode, [block], block_mapping, 0, 1)
            elif index == 13:
                traverse_indirect_reference(inode, [block], block_mapping, 0, 2)
            elif index == 14:
                traverse_indirect_reference(inode, [block], block_mapping, 0, 3)

    for block in block_mapping:
        output_string = ""
        if len(block_mapping[block]) > 1:
            dec_val = str(int(block, 16))
            output_string += "MULTIPLY REFERENCED BLOCK < " + dec_val + " > BY "
            for reference in block_mapping[block]:
                output_string += "INODE < " + reference[0] + " > ENTRY < " + reference[1] + " > "
            sys.stdout.write(output_string[:-1] + "\n") 


def unallocated_inodes():
    inode_num_set = set()
    for inode in inodes:
        inode_num_set.add(inode.inode_number)
    for directory in directories:
        if directory.inode_num_file not in inode_num_set:
            sys.stdout.write("UNALLOCATED INODE < " + directory.inode_num_file + " > REFERENCED BY DIRECTORY < " +
                             directory.parent_inode_num + " > ENTRY < " + directory.entry_num + " >\n")


def missing_inodes():
    global total_inodes, inodes_per_group
    total_set = set()
    for i in xrange(11, total_inodes + 1):
        total_set.add(str(i))
    found_set = set()
    for directory in directories:
        if int(directory.inode_num_file) > 10:
            found_set.add(directory.inode_num_file)
    found_set = found_set.union(free_inodes)
    missing_set = total_set.difference(found_set)
    block_inode_list = sorted([int(x, 16) for x in inode_bitmap_nums])
    for val in missing_set:
        start = 0
        end = inodes_per_group
        index = 0
        while not start < int(val) < end:
            start += inodes_per_group
            end += inodes_per_group
            index += 1
        sys.stdout.write("MISSING INODE < " + val + " > SHOULD BE IN FREE LIST < " + str(block_inode_list[index]) + " "
                         ">\n")


def incorrect_link_count():
    for inode in inodes:
        inode_num = inode.inode_number
        stated_links_count = int(inode.links_count)
        actual_count = 0
        for directory in directories:
            if directory.inode_num_file == inode_num:
                actual_count += 1
        if actual_count != stated_links_count:
            sys.stdout.write("LINKCOUNT < " + inode_num + " > IS < " + str(stated_links_count) + " > SHOULD BE < "
                             + str(actual_count) + " >\n")


def incorrect_directory_entry():
    for index, directory in enumerate(directories):
        if int(directory.parent_inode_num) < 11:
            continue
        if directory.name == "\".\"":
            if directory.inode_num_file != directory.parent_inode_num:
                sys.stdout.write("INCORRECT ENTRY IN < " + directory.parent_inode_num + " > NAME < . > LINK TO < "
                                 + directory.inode_num_file + " > SHOULD BE < " + directory.parent_inode_num + " >\n")
        elif directory.name == "\"..\"":
            start = index - 1
            while start >= 0 and directories[start].parent_inode_num == directory.parent_inode_num:
                start -= 1
            while start >= 0 and directories[start].inode_num_file != directory.parent_inode_num:
                start -= 1
            should_be = directories[start].parent_inode_num
            if directory.inode_num_file != should_be:
                sys.stdout.write("INCORRECT ENTRY IN < " + directory.parent_inode_num + " > NAME < .. > LINK TO < "
                                 + directory.inode_num_file + " > SHOULD BE < " + should_be + " >\n")


def invalid_blocks():
    for inode in inodes:
        traversed = [0]
        for index, block in enumerate(inode.blocks):
            if traversed[0] >= inode.num_blocks:
                break
            block = int(block, 16)
            if block == 0 or block > largest_block_number:
                sys.stdout.write("INVALID BLOCK < " + str(block) + " > IN INODE < " + inode.inode_number + " > ENTRY < "
                                 + str(index) + " >\n")
            elif index == 12:
                traverse_indirect_blocks(inode, [hex(block)[2:]], 0, 1, traversed)
            elif index == 13:
                traverse_indirect_blocks(inode, [hex(block)[2:]], 0, 2, traversed)
            elif index == 14:
                traverse_indirect_blocks(inode, [hex(block)[2:]], 0, 3, traversed)

            traversed[0] += 1


def setup():
    with open("group.csv", "r") as group:
        for line in group:
            line = line.rstrip()
            values = line.split(',')
            block_bitmap_nums.add(values[5])
            inode_bitmap_nums.add(values[4])
    with open("bitmap.csv", "r") as bitmap:
        for line in bitmap:
            line = line.rstrip()
            values = line.split(',')
            if values[0] in block_bitmap_nums:
                free_blocks.add(values[1])
            else:
                free_inodes.add(values[1])
    with open("inode.csv", "r") as inode:
        for line in inode:
            line = line.rstrip()
            values = line.split(',')
            inodes.append(Inode(values[0], values[11:], values[5], values[10]))
    with open("indirect.csv", "r") as indirect:
        for line in indirect:
            line = line.rstrip()
            values = line.split(',')
            if values[0] not in indirect_blocks:
                indirect_blocks[values[0]] = []
            indirect_blocks[values[0]].append([values[1], values[2]])
    with open("directory.csv", "r") as directory:
        for line in directory:
            line = line.rstrip()
            values = line.split(',')
            directories.append(Directory(values[0], values[1], values[4], values[5]))
    with open("super.csv", "r") as super_file:
        for line in super_file:
            line = line.rstrip()
            values = line.split(',')
            global inodes_per_group, total_inodes, largest_block_number
            inodes_per_group = int(values[6])
            total_inodes = int(values[1])
            largest_block_number = int(values[2])


def main():
    setup()
    unallocated_blocks()
    duplicated_referenced_blocks()
    unallocated_inodes()
    missing_inodes()
    incorrect_link_count()
    incorrect_directory_entry()
    invalid_blocks()
    sys.stdout.flush()

if __name__ == '__main__':
    main()
