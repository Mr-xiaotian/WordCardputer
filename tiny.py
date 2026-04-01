from celestialvault.instances.inst_file import FileTree


if __name__ == "__main__":
    dir1 = r'D:\Documents\Arduino\WordCardputer\words_study'
    dir2 = r'E:\words_study'
    exclude_dirs = []
    exclude_exts = [".db"]

    tree1 = FileTree.build_from_path(dir1, exclude_dirs = None, exclude_exts = exclude_exts, max_depth = 5)
    tree2 = FileTree.build_from_path(dir2, exclude_dirs = None, exclude_exts = exclude_exts, max_depth = 5)

    diff = tree1.compare_with(tree2, True)
    diff.print_diff_tree()

    diff.sync_dirs("->")