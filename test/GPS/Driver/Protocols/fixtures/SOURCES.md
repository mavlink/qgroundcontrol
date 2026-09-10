# Independent receiver fixtures

Unmodified frame slices from BSD-3-Clause upstream test recordings. The adjacent licenses apply
also to the duplicate `upstream-*` fuzz seeds in `../corpus`. No upstream runtime dependency is required.
Expected values were independently decoded using these pinned Python implementations.

| Files | Source | Byte offset and length |
| --- | --- | --- |
| nav-pvt.ubx | [pyubx2 NAV recording](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/pygpsdata-NAV.log) | 0, 100 |
| nav-dop.ubx | Same NAV recording | 1906, 26 |
| nav-sat.ubx | Same NAV recording | 758, 532 |
| nav-hpposllh.ubx | [pyubx2 HPPOS recording](https://github.com/semuconsulting/pyubx2/blob/9eadeeefc25162206a9200eea1a90bbb8ca315b9/tests/pygpsdata-NAVHPPOS.log) | 36, 44 |
| pvt-geodetic.sbf | [pysbf2 geodetic recording](https://github.com/semuconsulting/pysbf2/blob/7ea5e3aecb1349396d47f80938cfe20c461af95b/tests/pygpsdata_x5_pvtgeod.log) | 0, 96 |

The fixture test checks units, signs, precision, counted satellites, fragmented delivery and checksum rejection.
Epoch tests may deliberately replace iTOW and recompute the checksum; checked-in bytes remain unchanged.
