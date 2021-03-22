# Setup Environment
- run `ansible-playbook -K -i ./ansible_inventory ./ansible_provisions.yml`
- This command should be executed at `<project root>/devenv` cuz it uses `playbook_dir` variable.

# Setup EDK2
- run `cd ~/edk2`
- run `ln -s <project root>/MikanLoaderPkg ./`
- edit `Conf/target.txt` as below.
```
ACTIVE_PLATFORM = MikanLoaderPkg/MikanLoaderPkg.dsc
TARGET = DEBUG
TARGET_ARCH = X64
TOOL_CHAIN_TAG = CLANG38
```
- run `source ./edksetup.sh && build`
