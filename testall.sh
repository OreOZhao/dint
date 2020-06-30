

./queries single_packed_dint and single_packed_dint.bin < ../new/Million_QueryID

./queries single_overlapped_dint and single_overlapped_dint.bin < ../new/Million_QueryID

./queries multi_packed_dint and multi_packed_dint.bin < ../new/Million_QueryID

./queries multi_overlapped_dint and multi_overlapped_dint.bin < ../new/Million_QueryID

./encode single_rect_dint ../new/Gov2.docs --dict dict.Gov2.docs.rectangular.DSF-65536-16 --out test.bin
./decode single_rect_dint test.bin --dict dict.Gov2.docs.rectangular.DSF-65536-16
./encode single_rect_dint ../new/Gov2.freqs --dict dict.Gov2.freqs.rectangular.DSF-65536-16 --out test.bin
./decode single_rect_dint test.bin --dict dict.Gov2.freqs.rectangular.DSF-65536-16


./encode single_packed_dint ../new/Gov2.docs --dict dict.Gov2.docs.single_packed.DSF-65536-16 --out test.bin
./decode single_packed_dint test.bin --dict dict.Gov2.docs.single_packed.DSF-65536-16
./encode single_packed_dint ../new/Gov2.freqs --dict dict.Gov2.freqs.single_packed.DSF-65536-16 --out test.bin
./decode single_packed_dint test.bin --dict dict.Gov2.freqs.single_packed.DSF-65536-16

./encode multi_packed_dint ../new/Gov2.docs --dict dict.Gov2.docs.multi_packed.DSF-65536-16 --out test.bin
./decode multi_packed_dint test.bin --dict dict.Gov2.docs.multi_packed.DSF-65536-16
./encode multi_packed_dint ../new/Gov2.freqs --dict dict.Gov2.freqs.multi_packed.DSF-65536-16 --out test.bin
./decode multi_packed_dint test.bin --dict dict.Gov2.freqs.multi_packed.DSF-65536-16


./encode single_overlapped_dint ../new/Gov2.docs --dict dict.Gov2.docs.single_overlapped.DSF-65536-16 --out test.bin
./decode single_overlapped_dint test.bin --dict dict.Gov2.docs.single_overlapped.DSF-65536-16
./encode single_overlapped_dint ../new/Gov2.freqs --dict dict.Gov2.freqs.single_overlapped.DSF-65536-16 --out test.bin
./decode single_overlapped_dint test.bin --dict dict.Gov2.freqs.single_overlapped.DSF-65536-16

./encode multi_overlapped_dint ../new/Gov2.docs --dict dict.Gov2.docs.multi_overlapped.DSF-65536-16 --out test.bin
./decode multi_overlapped_dint test.bin --dict dict.Gov2.docs.multi_overlapped.DSF-65536-16
./encode multi_overlapped_dint ../new/Gov2.freqs --dict dict.Gov2.freqs.multi_overlapped.DSF-65536-16 --out test.bin
./decode multi_overlapped_dint test.bin --dict dict.Gov2.freqs.multi_overlapped.DSF-65536-16