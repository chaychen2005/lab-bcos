/**
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 *
 * @brief
 *
 * @file FakeBlock.h
 * @author: yujiechen
 * @date 2018-09-21
 */
#pragma once
#include <libdevcrypto/Common.h>
#include <libethcore/Block.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/CommonJS.h>
#include <libethcore/Transaction.h>
#include <boost/test/unit_test.hpp>

using namespace dev;
using namespace dev::eth;

namespace dev
{
namespace test
{
class FakeBlock
{
public:
    /// for normal case test
    FakeBlock(size_t size, Secret const& sec = KeyPair::create().secret())
    {
        m_sec = sec;
        FakeBlockHeader();
        FakeSigList(size);
        FakeTransaction(size);
        FakeTransactionReceipt(size);
        m_block.setSigList(m_sigList);
        m_block.setTransactions(m_transaction);
        m_block.setBlockHeader(m_blockHeader);
        m_block.setTransactionReceipts(m_transactionReceipt);
        BOOST_CHECK(m_transaction == m_block.transactions());
        BOOST_CHECK(m_sigList == m_block.sigList());
        BOOST_CHECK(m_blockHeader = m_block.header());
        m_block.encode(m_blockData);
    }

    /// for empty case test
    FakeBlock()
    {
        m_block.setBlockHeader(m_blockHeader);
        m_block.encode(m_blockData);
    }

    /// fake invalid block data
    bytes FakeInvalidBlockData(size_t index, size_t size)
    {
        bytes result;
        RLPStream s;
        s.appendList(4);
        FakeBlockHeader();
        FakeSigList(size);
        FakeTransaction(size);
        unsigned fake_value = 10;
        // s << fake_value << fake_value << fake_value << fake_value;
        if (index == 1)
            s.append(fake_value);
        else
            s.appendRaw(m_blockHeaderData);
        if (index == 2)
            s.append(fake_value);
        else
            s.appendRaw(m_transactionData);
        s.append(m_blockHeader.hash());
        s.appendVector(m_sigList);
        s.swapOut(result);
        return result;
    }

    /// check exception conditions related about decode and encode
    void CheckInvalidBlockData(size_t size)
    {
        FakeBlockHeader();
        m_block.setBlockHeader(m_blockHeader);
        BOOST_CHECK_THROW(m_block.decode(ref(m_blockHeaderData)), InvalidBlockFormat);
        BOOST_REQUIRE_NO_THROW(m_block.encode(m_blockData));
        m_block.header().setGasUsed(u256(3000000000));
        m_blockHeader.encode(m_blockHeaderData);
        BOOST_CHECK_THROW(m_block.encode(m_blockData), TooMuchGasUsed);
        /// construct invalid block format
        for (size_t i = 1; i < 3; i++)
        {
            bytes result_bytes = FakeInvalidBlockData(i, size);
            BOOST_CHECK_THROW(m_block.decode(ref(result_bytes)), InvalidBlockFormat);
        }
    }

    /// fake block header
    void FakeBlockHeader()
    {
        m_blockHeader.setParentHash(sha3("parent"));
        m_blockHeader.setRoots(sha3("transactionRoot"), sha3("receiptRoot"), sha3("stateRoot"));
        m_blockHeader.setLogBloom(LogBloom(0));
        m_blockHeader.setNumber(int64_t(0));
        m_blockHeader.setGasLimit(u256(3000000));
        m_blockHeader.setGasUsed(u256(100000));
        uint64_t current_time = 100000;  // utcTime();
        m_blockHeader.setTimestamp(current_time);
        m_blockHeader.appendExtraDataArray(jsToBytes("0x1020"));
        m_blockHeader.setSealer(u256(12));
        std::vector<h512> sealer_list;
        for (unsigned int i = 0; i < 10; i++)
        {
            sealer_list.push_back(toPublic(Secret(h256(i))));
        }
        m_blockHeader.setSealerList(sealer_list);
    }

    /// fake sig list
    void FakeSigList(size_t size)
    {
        /// set sig list
        Signature sig;
        h256 block_hash;
        m_sigList.clear();
        for (size_t i = 0; i < size; i++)
        {
            block_hash = sha3(toString("block " + i));
            sig = sign(m_sec, block_hash);
            m_sigList.push_back(std::make_pair(u256(block_hash), sig));
        }
    }

    /// fake transactions
    void FakeTransaction(size_t size)
    {
        RLPStream txs;
        txs.appendList(size);
        m_transaction.resize(size);
        fakeSingleTransaction();
        for (size_t i = 0; i < size; i++)
        {
            m_transaction[i] = m_singleTransaction;
            bytes trans_data;
            m_transaction[i].encode(trans_data);
            txs.appendRaw(trans_data);
        }
        txs.swapOut(m_transactionData);
    }

    void FakeTransactionReceipt(size_t size)
    {
        m_transactionReceipt.resize(size);
        for (size_t i = 0; i < size; ++i)
        {
            m_transactionReceipt[i] = m_singleTransactionReceipt;
        }
    }

    /// fake single transaction
    void fakeSingleTransaction()
    {
        u256 value = u256(100);
        u256 gas = u256(100000000);
        u256 gasPrice = u256(0);
        Address dst;
        std::string str = "test transaction";
        bytes data(str.begin(), str.end());
        m_singleTransaction = Transaction(value, gasPrice, gas, dst, data, 2);
        SignatureStruct sig = dev::sign(m_sec, m_singleTransaction.sha3(WithoutSignature));
        /// update the signature of transaction
        m_singleTransaction.updateSignature(sig);
    }

    void fakeSingleTransactionReceipt()
    {
        h256 root = h256("0x1024");
        u256 gasUsed = u256(10000);
        LogEntries logEntries = LogEntries();
        u256 status = u256(1);
        bytes outputBytes = bytes();
        Address address = toAddress(KeyPair::create().pub());
        m_singleTransactionReceipt =
            TransactionReceipt(root, gasUsed, logEntries, status, outputBytes, address);
    }

    Block& getBlock() { return m_block; }
    BlockHeader& getBlockHeader() { return m_blockHeader; }
    bytes& getBlockHeaderData() { return m_blockHeaderData; }
    bytes& getBlockData() { return m_blockData; }

public:
    Secret m_sec;
    Block m_block;
    BlockHeader m_blockHeader;
    Transactions m_transaction;
    Transaction m_singleTransaction;
    TransactionReceipts m_transactionReceipt;
    TransactionReceipt m_singleTransactionReceipt;
    std::vector<std::pair<u256, Signature>> m_sigList;
    bytes m_blockHeaderData;
    bytes m_blockData;
    bytes m_transactionData;
};

}  // namespace test
}  // namespace dev
