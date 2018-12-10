/*
    This file is part of FISCO-BCOS.

    FISCO-BCOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FISCO-BCOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "libdevcrypto/Hash.h"
#include <libdevcore/Common.h>
#include <libdevcore/easylog.h>
#include <libdevcrypto/AES.h>
#include <libdevcrypto/Common.h>
#include <test/tools/libutils/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::test;

// BOOST_AUTO_TEST_SUITE(Crypto)

// BOOST_FIXTURE_TEST_SUITE(AES, TestOutputHelperFixture)
// #if FISCO_GM
// // BOOST_AUTO_TEST_CASE(aesCBCEncrypt)
// // {
// //     LOG(DEBUG) << "gm aesCBCEncrypt";
// //     bytesConstRef plainData = "test";
    
// //     bytesConstRef ivData; 
// //     string const& keyData = fromHex(
// //         "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     int keyLen = length(keyData);
// //     bytes enData = aesCBCEncrypt(plainData, keyData, keyLen, ivData);
// //     // cout << "plainData = test || encode =" << toHex(enData) << endl;
// //     // bytes dnData = aesCBCDecrypt(enData, keyData, keyLen, ivData);
// //     // cout << "plainData = test || encode =" << toHex(dnData) << endl;
// //     BOOST_REQUIRE_EQUAL("test",enData);
// }

// // BOOST_AUTO_TEST_CASE(aesCBCDecrypt)
// // {
// //     LOG(DEBUG) << "gm aesCBCDecrypt";
// //     bytesConstRef plainData = "test";
    
// //     bytesConstRef ivData; 
// //     string const& keyData = fromHex(
// //         "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     int keyLen = length(keyData);
// //     bytes dnData = aesCBCDecrypt(enData, keyData, keyLen, ivData);

// //     BOOST_REQUIRE_EQUAL("test",dnData);
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptWrongSeed)
// // {
// //     LOG(DEBUG) << "gm aesCBCDecrypt";
// //     bytesConstRef plainData = "test";
    
// //     bytesConstRef ivData; 
// //     string const& keyData = fromHex(
// //         "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     int keyLen = length(keyData);
// //     bytes dnData = aesCBCDecrypt(enData, keyData, keyLen, ivData);

// //     BOOST_REQUIRE_EQUAL("test",dnData);
// // }

// // BOOST_AUTO_TEST_CASE(aesCBCDecrypt)
// // {
// //     LOG(DEBUG) << "gm aesCBCEncrypt";
// //     bytesConstRef plainData = ;
// //     int keyLen;
// //     bytesConstRef ivData; 
// //     std::string const& keyData;
// //     bytes seed = fromHex(
// //         "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     KeyPair kp(sha3Secure(aesCBCDecrypt(&seed, "test")));
// //     BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") != kp.address());
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptWrongSeed)
// // {
// //     LOG(DEBUG) << "gm AesDecryptWrongSeed";
// //     bytes seed = fromHex(
// //         "badaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     KeyPair kp(sha3Secure(aesCBCDecrypt(&seed, "test")));
// //     BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") != kp.address());
// // }
// #else
// // BOOST_AUTO_TEST_CASE(aesCBCDecrypt)
// // {
// //     LOG(DEBUG) << "aesCBCDecrypt";
// //     bytes seed = fromHex(
// //         "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     KeyPair kp(sha3Secure(aesCBCDecrypt(&seed, "test")));
// //     BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") == kp.address());
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptWrongSeed)
// // {
// //     LOG(DEBUG) << "AesDecryptWrongSeed";
// //     bytes seed = fromHex(
// //         "badaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     KeyPair kp(sha3Secure(aesCBCDecrypt(&seed, "test")));
// //     BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") != kp.address());
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptWrongPassword)
// // {
// //     LOG(DEBUG) << "AesDecryptWrongPassword";
// //     bytes seed = fromHex(
// //         "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     KeyPair kp(sha3Secure(aesCBCDecrypt(&seed, "badtest")));
// //     BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") != kp.address());
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptFailInvalidSeed)
// // {
// //     LOG(DEBUG) << "AesDecryptFailInvalidSeed";
// //     bytes seed = fromHex(
// //         "xdbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
// //         "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
// //         "96fce7ffc621");
// //     BOOST_CHECK(bytes() == aesCBCDecrypt(&seed, "test"));
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptFailInvalidSeedSize)
// // {
// //     LOG(DEBUG) << "AesDecryptFailInvalidSeedSize";
// //     bytes seed = fromHex("000102030405060708090a0b0c0d0e0f");
// //     BOOST_CHECK(bytes() == aesCBCDecrypt(&seed, "test"));
// // }

// // BOOST_AUTO_TEST_CASE(AesDecryptFailInvalidSeed2)
// // {
// //     LOG(DEBUG) << "AesDecryptFailInvalidSeed2";
// //     bytes seed = fromHex("000102030405060708090a0b0c0d0e0f000102030405060708090a0b0c0d0e0f");
// //     BOOST_CHECK(bytes() == aesCBCDecrypt(&seed, "test"));
// // }
// #endif
// BOOST_AUTO_TEST_SUITE_END()

// BOOST_AUTO_TEST_SUITE_END()
