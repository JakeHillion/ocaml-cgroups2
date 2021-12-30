CLEANUP = rm -f

C_COMPILER=clang

UNITY_ROOT=../extern/Unity

CFLAGS  = -std=c17

INC_DIRS=-I$(UNITY_ROOT)/src

SRC_NAMESPACES_FS_UNSHARE=$(UNITY_ROOT)/src/unity.c namespaces/fs/TestUnshare.c test_runners/namespaces_fs_TestUnshare_Runner.c
SRC_NAMESPACES_FS_CLONE=$(UNITY_ROOT)/src/unity.c namespaces/fs/TestClone.c test_runners/namespaces_fs_TestClone_Runner.c

TARGET_NAMESPACES_FS_UNSHARE=target/test_namespaces_fs_unshare
TARGET_NAMESPACES_FS_CLONE=target/test_namespaces_fs_clone

all: clean default

default: $(SRC_NAMESPACES_FS_UNSHARE) $(SRC_NAMESPACES_FS_CLONE)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_FS_UNSHARE) -o $(TARGET_NAMESPACES_FS_UNSHARE)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_FS_CLONE) -o $(TARGET_NAMESPACES_FS_CLONE)

	@echo
	@echo Finished preparing tests, running now.
	@echo

	- ./$(TARGET_NAMESPACES_FS_UNSHARE)
	- ./$(TARGET_NAMESPACES_FS_CLONE)

test_runners/namespaces_fs_TestUnshare_Runner.c: namespaces/fs/TestUnshare.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/fs/TestUnshare.c test_runners/namespaces_fs_TestUnshare_Runner.c

test_runners/namespaces_fs_TestClone_Runner.c: namespaces/fs/TestClone.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/fs/TestClone.c test_runners/namespaces_fs_TestClone_Runner.c

clean:
	$(CLEANUP) $(TARGET_NAMESPACES_FS_UNSHARE) $(TARGET_NAMESPACES_FS_CLONE)
	mkdir -p test_runners/ target/
