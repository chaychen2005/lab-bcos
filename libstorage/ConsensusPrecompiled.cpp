/*
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
 */
/** @file ConsensusPrecompiled.cpp
 *  @author chaychen
 *  @date 20181211
 */
#include "ConsensusPrecompiled.h"
#include "libstorage/EntriesPrecompiled.h"
#include "libstorage/TableFactoryPrecompiled.h"
#include <libdevcore/easylog.h>
#include <libethcore/ABI.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
using namespace dev;
using namespace dev::blockverifier;
using namespace dev::storage;

/*
contract ConsensusSystemTable
{
    function addMiner(string nodeID) public returns(int256);
    function addObserver(string nodeID) public returns(int256);
    function remove(string nodeID) public returns(int256);
}
*/

const char* const CSS_METHOD_ADD_MINER = "addMiner(string)";
const char* const CSS_METHOD_ADD_SER = "addObserver(string)";
const char* const CSS_METHOD_REMOVE = "remove(string)";

ConsensusPrecompiled::ConsensusPrecompiled()
{
    name2Selector[CSS_METHOD_ADD_MINER] = getFuncSelector(CSS_METHOD_ADD_MINER);
    name2Selector[CSS_METHOD_ADD_SER] = getFuncSelector(CSS_METHOD_ADD_SER);
    name2Selector[CSS_METHOD_REMOVE] = getFuncSelector(CSS_METHOD_REMOVE);
}

bytes ConsensusPrecompiled::call(
    ExecutiveContext::Ptr context, bytesConstRef param, Address const& origin)
{
    STORAGE_LOG(TRACE) << "this: " << this << " call CRUD:" << toHex(param);

    // parse function name
    uint32_t func = getParamFunc(param);
    bytesConstRef data = getParamData(param);

    STORAGE_LOG(DEBUG) << "func:" << std::hex << func;

    dev::eth::ContractABI abi;
    bytes out;
    u256 count = 0;

    showConsensusTable(context);


    if (func == name2Selector[CSS_METHOD_ADD_MINER])
    {
        // addMiner(string)
        std::string nodeID;
        abi.abiOut(data, nodeID);
        // Uniform lowercase nodeID
        boost::to_lower(nodeID);
        STORAGE_LOG(DEBUG) << "ConsensusPrecompiled addMiner params:" << nodeID;
        if (nodeID.size() != 128u)
        {
            STORAGE_LOG(DEBUG) << "ConsensusPrecompiled nodeID length error. " << nodeID;
        }
        else
        {
            storage::Table::Ptr table = openTable(context, SYS_MINERS);

            auto condition = table->newCondition();
            condition->EQ(NODE_KEY_NODEID, nodeID);
            auto entries = table->select(PRI_KEY, condition);
            auto entry = table->newEntry();
            entry->setField(NODE_TYPE, NODE_TYPE_MINER);
            entry->setField(PRI_COLUMN, PRI_KEY);
            entry->setField(NODE_KEY_ENABLENUM,
                boost::lexical_cast<std::string>(context->blockInfo().number + 1));

            if (entries.get())
            {
                if (entries->size() == 0u)
                {
                    entry->setField(NODE_KEY_NODEID, nodeID);
                    count = table->insert(PRI_KEY, entry, getOptions(origin));
                    STORAGE_LOG(DEBUG)
                        << "ConsensusPrecompiled new miner node, nodeID : " << nodeID;
                }
                else
                {
                    count = table->update(PRI_KEY, entry, condition, getOptions(origin));
                    STORAGE_LOG(DEBUG)
                        << "ConsensusPrecompiled change to miner, nodeID : " << nodeID;
                }
            }

            out = abi.abiIn("", count);
            STORAGE_LOG(DEBUG) << "ConsensusPrecompiled addMiner result:" << toHex(out);
        }
    }
    else if (func == name2Selector[CSS_METHOD_ADD_SER])
    {
        // addObserver(string)
        std::string nodeID;
        abi.abiOut(data, nodeID);
        // Uniform lowercase nodeID
        boost::to_lower(nodeID);
        STORAGE_LOG(DEBUG) << "ConsensusPrecompiled addObserver params:" << nodeID;
        if (nodeID.size() != 128u)
        {
            STORAGE_LOG(DEBUG) << "ConsensusPrecompiled nodeID length error. " << nodeID;
        }
        else
        {
            storage::Table::Ptr table = openTable(context, SYS_MINERS);

            auto condition = table->newCondition();
            condition->EQ(NODE_KEY_NODEID, nodeID);
            auto entries = table->select(PRI_KEY, condition);
            auto entry = table->newEntry();
            entry->setField(NODE_TYPE, NODE_TYPE_OBSERVER);
            entry->setField(PRI_COLUMN, PRI_KEY);
            entry->setField(NODE_KEY_ENABLENUM,
                boost::lexical_cast<std::string>(context->blockInfo().number + 1));

            if (entries.get())
            {
                if (entries->size() == 0u)
                {
                    entry->setField(NODE_KEY_NODEID, nodeID);
                    count = table->insert(PRI_KEY, entry, getOptions(origin));
                    STORAGE_LOG(DEBUG)
                        << "ConsensusPrecompiled new observer node, nodeID : " << nodeID;
                }
                else if (!checkIsLastMiner(table, nodeID))
                {
                    count = table->update(PRI_KEY, entry, condition, getOptions(origin));
                    STORAGE_LOG(DEBUG)
                        << "ConsensusPrecompiled change to observer, nodeID : " << nodeID;
                }
            }
            else
            {
                STORAGE_LOG(WARNING) << "ConsensusPrecompiled select SYS_MINERS table failed.";
            }

            out = abi.abiIn("", count);
            STORAGE_LOG(DEBUG) << "ConsensusPrecompiled addObserver result:" << toHex(out);
        }
    }
    else if (func == name2Selector[CSS_METHOD_REMOVE])
    {
        // remove(string)
        std::string nodeID;
        abi.abiOut(data, nodeID);
        // Uniform lowercase nodeID
        boost::to_lower(nodeID);
        STORAGE_LOG(DEBUG) << "ConsensusPrecompiled remove params:" << nodeID;
        if (nodeID.size() != 128u)
        {
            STORAGE_LOG(DEBUG) << "ConsensusPrecompiled nodeID length error. " << nodeID;
        }
        else
        {
            storage::Table::Ptr table = openTable(context, SYS_MINERS);

            if (!checkIsLastMiner(table, nodeID))
            {
                auto condition = table->newCondition();
                condition->EQ(NODE_KEY_NODEID, nodeID);
                count = table->remove(PRI_KEY, condition, getOptions(origin));
                STORAGE_LOG(DEBUG) << "ConsensusPrecompiled remove one node: " << nodeID;

                out = abi.abiIn("", count);
            }
        }
    }
    else
    {
        STORAGE_LOG(ERROR) << "ConsensusPrecompiled error func:" << std::hex << func;
    }
    return out;
}

void ConsensusPrecompiled::showConsensusTable(ExecutiveContext::Ptr context)
{
    storage::Table::Ptr table = openTable(context, SYS_MINERS);
    if (!table)
    {
        STORAGE_LOG(WARNING) << "ConsensusPrecompiled open SYS_MINERS table failed.";
        return;
    }

    auto condition = table->newCondition();
    auto entries = table->select(PRI_KEY, condition);

    std::stringstream s;
    s << "ConsensusPrecompiled show table:\n";
    if (entries.get())
    {
        for (size_t i = 0; i < entries->size(); i++)
        {
            auto entry = entries->get(i);
            std::string name = entry->getField(PRI_COLUMN);
            std::string nodeID = entry->getField(NODE_KEY_NODEID);
            std::string type = entry->getField(NODE_TYPE);
            std::string enable = entry->getField(NODE_KEY_ENABLENUM);
            s << "ConsensusPrecompiled[" << i << "]:" << name << "," << nodeID << "," << type << ","
              << enable << "\n";
        }
    }
    STORAGE_LOG(TRACE) << s.str();
}

bool ConsensusPrecompiled::checkIsLastMiner(storage::Table::Ptr table, std::string const& nodeID)
{
    // Check is last miner or not.
    auto condition = table->newCondition();
    condition->EQ(NODE_TYPE, NODE_TYPE_MINER);
    auto entries = table->select(PRI_KEY, condition);
    if (entries->size() == 1u && entries->get(0)->getField(NODE_KEY_NODEID) == nodeID)
    {
        // The nodeID in param is the last miner, cannot be deleted.
        STORAGE_LOG(WARNING) << "ConsensusPrecompiled the nodeID in param is the last "
                                "miner, cannot be deleted.";
        return true;
    }
    return false;
}
