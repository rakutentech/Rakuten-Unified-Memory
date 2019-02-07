#!/bin/sh
#----
rm -rf $RUM_HOME/app/build_s3cmd_test
mkdir -p $RUM_HOME/app/build_s3cmd_test
cd $RUM_HOME/app/build_s3cmd_test
cmake ../s3cmd_test
make
cd -
echo "====================================="
echo "====== Built the S3CMD test app ====="
echo "====================================="
