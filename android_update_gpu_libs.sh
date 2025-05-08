#!/bin/bash
# This script is used to copy android gpu-top binaries
# from android out folder to vendor/nxp/fsl-proprietary.
# And auto generate a commit which contains current branch and commit info.
# Pre-condition:
# Need source/lunch under android source code root directory firstly.
# Need setup a branch to track remote branch and local branch name must be
# same as remote one.
# This script assmues every commit contains "Signed-off-by:***".
# Need sync libgpuperfcnt and gputop git before build and build under gputop git.

RED='\033[0;31m'
STD='\033[0;0m'

function update_bin()
{
    if [ -e "$SRC_PWD/$1" ];then
        cp $SRC_PWD/$1   $DST_PWD/$2 && \
        echo "update the file $DST_PWD/$2 based on $SRC_PWD/$1"
    else
        echo "$SRC_PWD/$1 not exits!"
    fi
}

# Once there's anything need to be copied, need add it here.
function update_all()
{
    update_bin "bin/gpu-top" "test/gputop/$GPU_VENDOR/gpu-top"
}

# Auto generate a commit which contains current branch and commit info.
# Commit message format is as below:
# MA-**** ****
# Source git: gputop.git
# Source branch: remotes/origin/****
# Source commit: ****
#
# MA-**** ****
# Source git: libgpuperfcnt.git
# Source branch: remotes/origin/****
# Source commit: ****
function auto_commit()
{
    cd ../libgpuperfcnt
    COMMIT_ID=`git rev-parse HEAD`
    COMMIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
    TEMP_FILE=`pwd`/temp_lib.txt
    git log -1 --pretty='%B' > ${TEMP_FILE}
    sed -i "s/Change-Id:.*//g" ${TEMP_FILE}
    sed -i "s/Signed-off-by.*/Source git: libgpuperfcnt.git\nSource branch: remotes\/origin\/${COMMIT_BRANCH}\nSource commit: ${COMMIT_ID}/g" ${TEMP_FILE}
    sed -i "s/ \[\#imx-.*\]//g" ${TEMP_FILE}

    cd ../gputop
    COMMIT_ID=`git rev-parse HEAD`
    COMMIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
    GPU_FILE=`pwd`/temp.txt
    git log -1 --pretty='%B' > ${GPU_FILE}
    sed -i "s/Change-Id:.*//g" ${GPU_FILE}
    sed -i "s/Signed-off-by.*/Source git: gputop.git\nSource branch: remotes\/origin\/${COMMIT_BRANCH}\nSource commit: ${COMMIT_ID}/g" ${GPU_FILE}
    sed -i "s/ \[\#imx-.*\]//g" ${GPU_FILE}

    cat ${TEMP_FILE} >> ${GPU_FILE}
    rm -rf ${TEMP_FILE}

    cd $DST_PWD/
    git add *
    git commit -s -F ${GPU_FILE}
    rm -rf ${GPU_FILE}
}

function build_gpu_top()
{
    if [ "$GPU_VENDOR" = "vsi" ]; then
        echo "PLease build gputop binary with the below cmd:"
        echo "MALI_GPU=0 mm -j24 DISABLE_FSL_PREBUILT=ALL"
    elif [ "$GPU_VENDOR" = "mali" ]; then
        echo "PLease build gputop binary with the below cmd:"
        echo "MALI_GPU=1 mm -j24 DISABLE_FSL_PREBUILT=ALL"
    else
        echo "Invalid GPU vendor specified for build: $GPU_VENDOR"
        exit 1
    fi
}

function update_gpu_top()
{
    SRC_PWD="$OUT/vendor"
    DST_PWD="$ANDROID_BUILD_TOP/vendor/nxp/fsl-proprietary"
    DRIVER_PWD=`pwd`

    update_all
    auto_commit
}

function help() {
bn=`basename $0`

cat << EOF

Usage: $bn <option> <gpu_vendor>
options:
  -h                displays this help message
  -build            display build command of gputop binary for specified vendor(vsi/mali)
  -update           update gputop binary for specified vendor(vsi/mali) and generate commit

gpu_vendor:
  vsi               use VSI GPU vendor
  mali              use Mali GPU vendor

Examples:
  $bn -h
  $bn -build vsi
  $bn -update vsi
EOF
}

if [ -z "$OUT" ] || [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Env is not ready! Please go to android source code root directory:"
    echo "source build/envsetup.sh"
    echo "lunch"
    exit 1
fi

if [ $# -lt 1 ]; then
    echo -e ${RED}no parameter specified, will directly exit after displaying help message${STD}
    help
    exit 1
fi

case "$1" in
    -h)
        help
        exit 0
        ;;
    -build|-update)
        if [ $# -lt 2 ]; then
            echo -e ${RED}missing gpu_vendor parameter, will directly exit after displaying help message${STD}
            help
            exit 1
        fi
        GPU_VENDOR="$2"
        ;;
    *)
        echo -e ${RED}invalid option: $1, will directly exit after displaying help message${STD}
        help
        exit 1
        ;;
esac

case "$1" in
    -build)
        build_gpu_top
        ;;
    -update)
        update_gpu_top
        ;;
esac
