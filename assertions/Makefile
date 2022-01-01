CLEANUP = rm -f

C_COMPILER=clang

UNITY_ROOT=../extern/Unity

CFLAGS  = -std=c17

INC_DIRS=-I$(UNITY_ROOT)/src

SRC_NAMESPACES_FS_UNSHARE=$(UNITY_ROOT)/src/unity.c namespaces/fs/TestUnshare.c test_runners/namespaces_fs_TestUnshare_Runner.c
TARGET_NAMESPACES_FS_UNSHARE=target/test_namespaces_fs_unshare

SRC_NAMESPACES_FS_CLONE=$(UNITY_ROOT)/src/unity.c namespaces/fs/TestClone.c test_runners/namespaces_fs_TestClone_Runner.c
TARGET_NAMESPACES_FS_CLONE=target/test_namespaces_fs_clone

SRC_NAMESPACES_FILES_CLONE=$(UNITY_ROOT)/src/unity.c namespaces/files/TestClone.c test_runners/namespaces_files_TestClone_Runner.c
TARGET_NAMESPACES_FILES_CLONE=target/test_namespaces_files_clone

SRC_NAMESPACES_MOUNT_CLONE=$(UNITY_ROOT)/src/unity.c namespaces/mount/TestClone.c test_runners/namespaces_mount_TestClone_Runner.c
TARGET_NAMESPACES_MOUNT_CLONE=target/test_namespaces_mount_clone


all: clean default

default: $(SRC_NAMESPACES_FS_UNSHARE) $(SRC_NAMESPACES_FS_CLONE) $(SRC_NAMESPACES_FILES_CLONE) $(SRC_NAMESPACES_MOUNT_CLONE)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_FS_UNSHARE) -o $(TARGET_NAMESPACES_FS_UNSHARE)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_FS_CLONE) -o $(TARGET_NAMESPACES_FS_CLONE)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_FILES_CLONE) -o $(TARGET_NAMESPACES_FILES_CLONE)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_MOUNT_CLONE) -o $(TARGET_NAMESPACES_MOUNT_CLONE)

	@echo
	@echo Finished preparing tests, running now.
	@echo

	sudo setcap cap_sys_admin+eip $(TARGET_NAMESPACES_MOUNT_CLONE)

	- ./$(TARGET_NAMESPACES_FS_UNSHARE)
	- ./$(TARGET_NAMESPACES_FS_CLONE)
	- ./$(TARGET_NAMESPACES_FILES_CLONE)
	- ./$(TARGET_NAMESPACES_MOUNT_CLONE)

test_runners/namespaces_fs_TestUnshare_Runner.c: namespaces/fs/TestUnshare.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/fs/TestUnshare.c test_runners/namespaces_fs_TestUnshare_Runner.c

test_runners/namespaces_fs_TestClone_Runner.c: namespaces/fs/TestClone.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/fs/TestClone.c test_runners/namespaces_fs_TestClone_Runner.c

test_runners/namespaces_files_TestClone_Runner.c: namespaces/files/TestClone.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/files/TestClone.c test_runners/namespaces_files_TestClone_Runner.c

test_runners/namespaces_mount_TestClone_Runner.c: namespaces/mount/TestClone.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/mount/TestClone.c test_runners/namespaces_mount_TestClone_Runner.c

clean:
	$(CLEANUP) $(TARGET_NAMESPACES_FS_UNSHARE) $(TARGET_NAMESPACES_FS_CLONE) $(TARGET_NAMESPACES_FILES_CLONE)
	mkdir -p test_runners/ target/
