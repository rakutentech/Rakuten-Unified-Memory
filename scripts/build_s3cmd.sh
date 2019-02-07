#!/bin/sh
#----
rm -rf $RUM_HOME/app/build_s3cmd
mkdir -p $RUM_HOME/app/build_s3cmd
cd $RUM_HOME/app/build_s3cmd
cmake ../s3cmd
make
cd -
echo "====================================="
echo "====== Built the S3CMD app =========="
echo "====================================="
