CLEANUP = rm -f

C_COMPILER=clang

UNITY_ROOT=../extern/Unity

CFLAGS  = -std=c17

INC_DIRS=-I$(UNITY_ROOT)/src

SRC_NAMESPACES_FS_1=$(UNITY_ROOT)/src/unity.c namespaces/fs/TestUnshare.c test_runners/namespaces_fs_TestUnshare_Runner.c
TARGET_NAMESPACES_FS_1=target/test_namespaces_fs

all: clean default

default: $(TARGET_NAMESPACES_FS_1)
	- ./$(TARGET_NAMESPACES_FS_1)

$(TARGET_NAMESPACES_FS_1): $(SRC_NAMESPACES_FS_1)
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_NAMESPACES_FS_1) -o $(TARGET_NAMESPACES_FS_1)

test_runners/namespaces_fs_TestUnshare_Runner.c: namespaces/fs/TestUnshare.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb namespaces/fs/TestUnshare.c test_runners/namespaces_fs_TestUnshare_Runner.c

clean:
	$(CLEANUP) $(TARGET_NAMESPACES_FS_1)
	mkdir -p test_runners/ target/
