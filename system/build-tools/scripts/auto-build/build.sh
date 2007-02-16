#!/bin/bash

if [ -z $LOG_DIR ]
then
  LOG_DIR=$HOME/Logs
  LOG=$LOG_DIR/std-out-err.log
fi
if [ -z $SOURCE_DIR ]
then
  SOURCE_DIR=$HOME/Source/Anonymous
fi
if [ -z $BUILD_DIR ]
then
  BUILD_DIR=$HOME/Build
fi

SYSTEM_LOG=$LOG_DIR/system-stdout.log
SYSTEM_FAILURE_LOG=$LOG_DIR/system-failures.log
SYSTEM_SUMMARY_LOG=$LOG_DIR/system-summary.log

BASE_LOG=$LOG_DIR/base-stdout.log
BASE_FAILURE_LOG=$LOG_DIR/base-failures.log
BASE_SUMMARY_LOG=$LOG_DIR/base-summary.log

# If you want to automatically upload the build to
# an FTP server, set the following environment variables
# in your .profile:
#
# FTP_SERVER
# FTP_USER
# FTP_PASSWD
#
# DANGER: This script will perform an "mdel *" command on the
# FTP server once it has logged in: you may not want that to
# happen!  Either use a dedicated user for builds, or remove
# the "mdel *" command below.

# Mark the start
printf "Build started at %s\n" "`date`" > $LOG

# Make sure we have the latest Builder
echo "Updating Builder"
build update 1>>$LOG 2>>$LOG
sync

# Build the 'system' profile
cd $BUILD_DIR/system
build prepare system

echo "image system"
image system 1>>$LOG 2>>$LOG
build log >> $SYSTEM_LOG
build log failures >> $SYSTEM_FAILURE_LOG
build log summary >> $SYSTEM_SUMMARY_LOG
sync

# Build the 'base' profile
echo "image base"
image base 1>>$LOG 2>>$LOG
build log > $BASE_LOG
build log failures > $BASE_FAILURE_LOG
build log summary > $BASE_SUMMARY_LOG
sync

# XXXKV: We have to build GCC 4.1.1 with GCC 3.4.3...
echo "Switching to GCC 3.4.3"
build install $HOME/Packages/gcc-3.4.3.bin.3.zip 1>>$LOG 2>>$LOG
sync
echo "Building GCC 4.1.1"
image gcc-libraries 1>>$LOG 2>>$LOG
build log >> $SYSTEM_LOG
build log failures >> $SYSTEM_FAILURE_LOG
build log summary >> $SYSTEM_SUMMARY_LOG
sync

# Grub also must be built with GCC 3.4.3
echo "Building grub-0.97"
image grub 1>>$LOG 2>>$LOG
build log >> $SYSTEM_LOG
build log failures >> $SYSTEM_FAILURE_LOG
build log summary >> $SYSTEM_SUMMARY_LOG
sync

echo "Switching to GCC 4.1.1"
build install $HOME/Packages/gcc-4.1.1.bin.2.zip 1>>$LOG 2>>$LOG
sync

# XXXKV: We have to build Xpdf with Freetype-2.1.10..
echo "Switching to Freetype-2.1.10"
build install $HOME/Packages/freetype-2.1.10.bin.1.zip 1>>$LOG 2>>$LOG
sync
echo "Building Xpdf"
image xpdf 1>>$LOG 2>>$LOG
build log >> $SYSTEM_LOG
build log failures >> $SYSTEM_FAILURE_LOG
build log summary >> $SYSTEM_SUMMARY_LOG
sync
echo "Switching to Freetype-2.2.1"
build install $HOME/Packages/freetype-2.2.1.bin.2.zip 1>>$LOG 2>>$LOG
sync

# XXXKV: Now on with the show...
cd stage/image
build scrub 1>>$LOG 2>>$LOG

# Mark the end
echo "Build complete"
printf "Build finished at %s\n" "`date`" >> $LOG

# Transfer the logs
FILES=`ls -1 $LOG_DIR | tr '\n' ' '`
if [ -n "$FTP_USER" ]
then
  echo "Transfering logs"
  cd $LOG_DIR
  ftp -n $FTP_SERVER << END
quote user $FTP_USER
quote pass $FTP_PASSWD
prompt
passive
mdel *
mput $FILES
quit
END
fi

sync
echo "Finished"
exit 0
