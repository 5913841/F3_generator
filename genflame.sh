sudo perf record -F 99 -g sudo ./build/THUGEN
sudo perf script -i perf.data &> perf.unfold
../FlameGraph/stackcollapse-perf.pl perf.unfold &> perf.folded
../FlameGraph/flamegraph.pl perf.folded &> perf.svg