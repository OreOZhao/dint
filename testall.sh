./dint

./queries single_packed_dint and single_packed_dint.bin < ../test/test_data/queries

./queries multi_packed_dint and multi_packed_dint.bin < ../test/test_data/queries

./encode single_rect_dint ../test/test_data/test_collection.docs --dict dict.test_collection.docs.single_rect.DSF-65536-16 --out test.bin
./decode single_rect_dint test.bin --dict dict.test_collection.docs.single_rect.DSF-65536-16
./encode single_rect_dint ../test/test_data/test_collection.freqs --dict dict.test_collection.freqs.single_rect.DSF-65536-16 --out test.bin
./decode single_rect_dint test.bin --dict dict.test_collection.freqs.single_rect.DSF-65536-16

./encode single_packed_dint ../test/test_data/test_collection.docs --dict dict.test_collection.docs.single_packed.DSF-65536-16 --out test.bin
./decode single_packed_dint test.bin --dict dict.test_collection.docs.single_packed.DSF-65536-16
./encode single_packed_dint ../test/test_data/test_collection.freqs --dict dict.test_collection.freqs.single_packed.DSF-65536-16 --out test.bin
./decode single_packed_dint test.bin --dict dict.test_collection.freqs.single_packed.DSF-65536-16

./encode multi_packed_dint ../test/test_data/test_collection.docs --dict dict.test_collection.docs.multi_packed.DSF-65536-16 --out test.bin
./decode multi_packed_dint test.bin --dict dict.test_collection.docs.multi_packed.DSF-65536-16
./encode multi_packed_dint ../test/test_data/test_collection.freqs --dict dict.test_collection.freqs.multi_packed.DSF-65536-16 --out test.bin
./decode multi_packed_dint test.bin --dict dict.test_collection.freqs.multi_packed.DSF-65536-16
