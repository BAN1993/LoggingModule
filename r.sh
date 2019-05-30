ulimit -c unlimited

rm -rf ./bin/core
rm -rf ./bin/testBuffer.log
rm -rf ./bin/testNew.log
rm -rf ./bin/do0
rm -rf ./bin/do1

make clean && make

cd bin

# 比较两种方式的效率
(./dotest 0 |tee do0) 
(./dotest 1 |tee do1)

# 执行多次,算算平均值
#./dotest 1
#./dotest 1
#./dotest 1
#./dotest 1
