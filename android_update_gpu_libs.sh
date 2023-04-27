#/bin/bash
# This script is used to copy android gpu-top binaries
# from android out folder to vendor/nxp/fsl-proprietary.
# And auto generate a commit which contains current branch and commit info.
# Pre-condition:
# Need source/lunch under android source code root directory firstly.
# Need setup a branch to track remote branch and local branch name must be
# same as remote one.
# This script assmues every commit contains "Signed-off-by:***".
# Need sync libgpuperfcnt and gputop git before build and build under gputop git.

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
    update_bin "bin/gpu-top" "test/bin/gpu-top"
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
	sed -i "s/ \[#imx-.*\]//g" ${TEMP_FILE}

	cd ../gputop
	COMMIT_ID=`git rev-parse HEAD`
    COMMIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
    GPU_FILE=`pwd`/temp.txt
    git log -1 --pretty='%B' > ${GPU_FILE}
    sed -i "s/Change-Id:.*//g" ${GPU_FILE}
    sed -i "s/Signed-off-by.*/Source git: gputop.git\nSource branch: remotes\/origin\/${COMMIT_BRANCH}\nSource commit: ${COMMIT_ID}/g" ${GPU_FILE}
    sed -i "s/ \[#imx-.*\]//g" ${GPU_FILE}
	
	cat ${TEMP_FILE} >> ${GPU_FILE}
	rm -rf ${TEMP_FILE}

    cd $DST_PWD/
    git add *
    git commit -s -F ${GPU_FILE}
    rm -rf ${GPU_FILE}
}

if [ -z $OUT ] && [ -z $ANDROID_BUILD_TOP ];then
    echo "Env is not ready! Please go to android source code root directory:"
    echo "source build/envsetup.sh"
    echo "lunch"
    exit
else
    SRC_PWD=$OUT/vendor
    DST_PWD=$ANDROID_BUILD_TOP/vendor/nxp/fsl-proprietary
    DRIVER_PWD=`pwd`
    update_all
    auto_commit
fi

