//
// Created by Иван Ильин on 25.05.2021.
//

#include "Server.h"
#include "engine/utils/Log.h"

void Server::broadcast() {
    sf::Packet updatePacket;
    updatePacket << MsgType::Update;

    for (auto& player : _players) {
        //player.second->setHealth(player.second->health() + (Time::time() - _lastBroadcast)/100);
        updatePacket << player.first << player.second->position().x() << player.second->position().y() << player.second->position().z() << player.second->health() << player.second->angle().y() << player.second->headAngle();
    }

    for (auto& player : _players) {
        _socket.send(updatePacket, player.first);
    }
}


void Server::processConnect(sf::Uint16 targetId) {
    sf::Packet sendPacket1, sendPacket2;
    sf::Packet extraPacket;

    // players init
    extraPacket << MsgType::NewClient << targetId;
    sendPacket1 << MsgType::Init << targetId;
    _players.insert({ targetId, std::make_shared<Player>() });
    for (const auto& player : _players)
    {
        sendPacket1 << player.first << player.second->position().x() << player.second->position().y() << player.second->position().z() << player.second->health();
        if (player.first != targetId)
            _socket.sendRely(extraPacket, player.first);
    }
    _socket.sendRely(sendPacket1, targetId);


    // bonuses init
    sendPacket2 << MsgType::InitBonuses;
    for(auto& bonus : _bonuses) {
        if(bonus.second.onTheMap)
            sendPacket2 << bonus.first << bonus.second.position.x() << bonus.second.position.y() << bonus.second.position.z();
    }
    _socket.sendRely(sendPacket2, targetId);

}

void Server::processClientUpdate(sf::Uint16 senderId, sf::Packet& packet) {
    double buf[5];

    packet >> buf[0] >> buf[1] >> buf[2] >> buf[3] >> buf[4];
    _players.at(senderId)->translateToPoint(Vec3D{ buf[0], buf[1], buf[2] });
    _players.at(senderId)->rotateToAngle(Vec3D{0, buf[3], 0});
    _players.at(senderId)->setHeadAngle(buf[4]);
}

void Server::processDisconnect(sf::Uint16 senderId) {
    sf::Packet sendPacket;

    sendPacket << MsgType::Disconnect << senderId;
    _players.erase(senderId);
    for (const auto& player : _players)
        _socket.sendRely(sendPacket, player.first);
}


void Server::processCustomPacket(MsgType type, sf::Packet& packet, sf::Uint16 senderId) {
    sf::Packet sendPacket;
    double dbuff[10];
    sf::Uint16 targetId;
    double damage;
    std::string tmp;
    double newHealth;

    switch (type) {
        case MsgType::Damage:
            packet >> targetId >> damage;
            newHealth = _players[targetId]->health() - damage;
            if(newHealth > 0) {
                _players[targetId]->setHealth(newHealth);
            }
            else {
                _players[targetId]->setFullHealth();
                _players[targetId]->setFullAbility();
                _players[targetId]->addDeath();
                _players[senderId]->addKill();

                sendPacket << MsgType::Kill << targetId << senderId;
                for (auto& player : _players)
                    _socket.send(sendPacket, player.first);
            }
            break;
        case MsgType::FireTrace:
            packet >> dbuff[0] >> dbuff[1] >> dbuff[2] >> dbuff[3] >> dbuff[4] >> dbuff[5];
            sendPacket << MsgType::FireTrace << dbuff[0] << dbuff[1] << dbuff[2] << dbuff[3] << dbuff[4] << dbuff[5];
            for (auto& player : _players) {
                if(player.first != senderId)
                    _socket.send(sendPacket, player.first);
            }

            break;
        case MsgType::RemoveBonus:
            packet >> tmp;

            if(tmp.find("Bonus_hill") != std::string::npos) {
                _players[senderId]->setFullHealth();
            }
            if(tmp.find("Bonus_ability") != std::string::npos) {
                _players[senderId]->setFullAbility();
            }

            _bonuses[tmp].onTheMap = false;
            _bonuses[tmp].lastTake = Time::time();
            sendPacket << MsgType::RemoveBonus << tmp;
            for (auto& player : _players) {
                if(player.first != senderId)
                    _socket.send(sendPacket, player.first);
            }
            break;
    }
}

void Server::processStop() {
    _players.clear();
    _bonuses.clear();
}

void Server::generateBonuses() {
    _bonuses.insert({"Bonus_gun_1", {Point4D(-10, -2, -15), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_gun_2", {Point4D(10, -2, 15), -2*_bonusRechargeTime, true}});

    _bonuses.insert({"Bonus_shotgun_1", {Point4D(-10, 13, -24), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_shotgun_2", {Point4D(10, 13, 24), -2*_bonusRechargeTime, true}});

    _bonuses.insert({"Bonus_ak47_1", {Point4D(-25, 30, 50), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_ak47_2", {Point4D(25, 30, -50), -2*_bonusRechargeTime, true}});

    _bonuses.insert({"Bonus_gold_ak47_1", {Point4D(-35, 80, 25), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_gold_ak47_2", {Point4D(35, 80, -25), -2*_bonusRechargeTime, true}});

    _bonuses.insert({"Bonus_rifle_1", {Point4D(40, -2, 45), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_rifle_2", {Point4D(-40, -2, -45), -2*_bonusRechargeTime, true}});

    _bonuses.insert({"Bonus_hill_1", {Point4D(-40, -2, 45), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_hill_2", {Point4D(40, -2, -45), -2*_bonusRechargeTime, true}});

    _bonuses.insert({"Bonus_ability_1", {Point4D(25, 18, -33), -2*_bonusRechargeTime, true}});
    _bonuses.insert({"Bonus_ability_2", {Point4D(-25, 18, 33), -2*_bonusRechargeTime, true}});
}

void Server::updateInfo() {
    for(auto& bonus : _bonuses) {
        if(!bonus.second.onTheMap && std::abs(Time::time() - bonus.second.lastTake) > _bonusRechargeTime) {
            sf::Packet sendPacket;
            sendPacket << MsgType::AddBonus << bonus.first << bonus.second.position.x() << bonus.second.position.y() << bonus.second.position.z();
            for (const auto& player : _players)
                _socket.sendRely(sendPacket, player.first);
            bonus.second.onTheMap = true;
        }
    }
}

Server::~Server() {
    processStop();
}
