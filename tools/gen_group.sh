#!/bin/bash
set -e
LOG_ERROR()
{
    content=${1}
    echo -e "\033[31m"${content}"\033[0m"
}

LOG_INFO()
{
    content=${1}
    echo -e "\033[32m"${content}"\033[0m"
}
function help()
{
echo "${1}"
    cat << EOF
Usage:
    -g <group id>               [Required] group id
    -d <node directory>         [Optional] default is nodes
    -l <node ip:node list>      [Required] eg. 127.0.0.1:0 1; 192.168.1.10:2 3
    -s <use storage state>      [Optional] default is mpt
    -h Help
e.g: # generate group2 from {127.0.0.1:node0}, {127.0.0.1:node1}, {192.168.1.10:node2}, {192.168.1.10 node3}
    bash gen_group.sh -g 2 -d nodes -l "127.0.0.1:0 1; 192.168.1.10:2 3"
EOF

exit 0
}

group_id=
ip_miner=
node_dir=nodes
state_type=mpt
function generate_group_ini()
{
    local nodeid_list="${1}"
    local output="${2}"
    cat << EOF > ${output}
;consensus configuration
[consensus]
;consensus type: only support PBFT now
consensusType=pbft
;the max number of transactions of a block
maxTransNum=1000
;the node id of leaders
$nodeid_list

;sync period time
[sync]
idleWaitMs=200

[storage]
;storage db type, now support leveldb 
type=LevelDB

[state]
;state type, now support mpt/storage
type=${state_type}



;genesis configuration
[genesis]
;used to mark the genesis block of this group
;mark=${group_id}

;txpool limit
[txPool]
limit=1000
EOF
}

function updateConfig()
{
    local configFile="${1}"
    local config="${2}"
    sed -i "/\[group\]/a$config" ${configFile} 
}
nodeidList=
function generateNodeIdList()
{
    local node_dir="${1}"
    local ip_miner="${2}"
    OLD_IFS="$IFS" 
    IFS=";" 
    ipList=($ip_miner)
    i=0
    for ip in ${ipList[*]};do
        IFS=" "
        ipMiners=`echo ${ip} | cut -d':' -f2`
        signleIp=`echo ${ip} | cut -d':' -f1`
        miners=(${ipMiners})
        IFS="$OLD_IFS" 
        local prefix=$node_dir"/node_"${signleIp}
        for miner in ${miners[*]};do
                if [ ! -f "${prefix}_${miner}/config.ini" ];then
                    LOG_ERROR "${prefix}_${miner}/config.ini doesn't exist!"
                    continue
                fi
                certDir=`cat ${prefix}_${miner}/config.ini | grep -w data_path | grep -v ";" | grep -v group | cut -d'=' -f2`
                if [ "${certDir}" == "" ];then
                    certDir="conf/"
                fi
                minerNodeId=`cat ${prefix}_${miner}/${certDir}/node.nodeid`
                if [ ! -f "${prefix}_${miner}/${certDir}/node.nodeid" ];then
                    LOG_ERROR "${prefix}_${miner}/${certDir}/node.nodeid doesn't exist!"
                    continue
                fi
                nodeidList="${nodeidList}node.${i}=${minerNodeId}"$'\n'
                i=$((i+1))
        done
    done
}

function generateGroupConfig()
{
    local groupId="${1}"
    local node_dir="${2}"
    local ip_miner="${3}"
    OLD_IFS="$IFS" 
    IFS=";" 
    ipList=($ip_miner) 
    generateNodeIdList "${node_dir}" "${ip_miner}"
    for ip in ${ipList[*]};do
        IFS=" "
        ipMiners=`echo ${ip} | cut -d':' -f2`
        signleIp=`echo ${ip} | cut -d':' -f1`
        miners=(${ipMiners})
        IFS="$OLD_IFS" 
        local prefix=$node_dir"/node_"${signleIp}
        # get nodeidList for specified machine
        for minerNode in ${miners[*]};do
            if [ ! -d "${prefix}_${minerNode}" ];then
                LOG_ERROR "Directory ${prefix}_${minerNode} doesn't exist!"
                continue
            fi
            groupConfigPath=`cat ${prefix}_${minerNode}/config.ini | grep group_config.${group_id} | grep -v ";" | cut -d'=' -f2`
            if [ "${groupConfigPath}" == "" ];then
                groupConfigPath=conf/
                mkdir -p ${groupConfigPath}
                updateConfig "${prefix}_${minerNode}/config.ini" "    group_config.${groupId}=conf/group.${groupId}.ini"
                groupConfigPath=${groupConfigPath}"/group."${groupId}".ini"
            fi
            groupConfigPath=${prefix}_${minerNode}/${groupConfigPath}
            generate_group_ini "${nodeidList}" "${groupConfigPath}"
        done
    done
}

function checkParam()
{
    if [ "${group_id}" == "" ];then
        LOG_ERROR "Must set group id"
        help
    fi
    if [ "${ip_miner}" == "" ];then
        LOG_ERROR "Must set Miner list"
        help
    fi
}

function main()
{
while getopts "g:d:l:sh" option;do
    case ${option} in
    g) group_id=${OPTARG};;
    #n) miner_list=${OPTARG};;
    d) node_dir=${OPTARG};;
    l) ip_miner=${OPTARG};;
    s) state_type=storage;;
    h) help;;
    esac
done
prefix=${node_dir}"/node_${ip}"
checkParam
generateGroupConfig "${group_id}" "${node_dir}" "${ip_miner}"
LOG_INFO "generate config for group."${group_id}" succ!"
}
main "$@"
