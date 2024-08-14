#include "mod/Config.h"
#include "mod/MyMod.h"

#include "ll/api/Config.h"
#include "ll/api/data/KeyValueDB.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "mc/entity/utilities/RailMovementUtility.h"
#include "mc/math/Vec3.h"
#include "mc/world/ActorUniqueID.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/IConstBlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/scores/Objective.h"
#include "mc/world/scores/ScoreInfo.h"
#include "mc/world/scores/Scoreboard.h"


#include "fmt/core.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>


using std::optional;

std::unique_ptr<ll::data::KeyValueDB> minecartDB;

void ModInit() {
    auto&       self           = my_mod::MyMod::getInstance().getSelf();
    auto&       logger         = self.getLogger();
    const auto& configFilePath = self.getConfigDir() / "config.json";
    if (!ll::config::loadConfig(config, configFilePath)) {
        logger.warn("加载配置文件失败: {}", configFilePath);
        logger.info("将生成默认配置文件...");

        if (!ll::config::saveConfig(config, configFilePath)) {
            logger.error("生成默认配置文件失败: {}", configFilePath);
        }
    }
    const auto& minecartDbPath = self.getDataDir() / "minecart";
    minecartDB                 = std::make_unique<ll::data::KeyValueDB>(minecartDbPath);
};

void Form(Player& player, const ActorUniqueID& actorUniqueId) {
    ll::form::SimpleForm form;
    form.setTitle("Minecart Speed Plus");
    std::for_each(config.moveMentList.begin(), config.moveMentList.end(), [&](const MoveMentData& data) {
        form.appendButton(data.name, [&](Player& player) {
            ll::form::ModalForm modalForm;
            modalForm.setTitle("Minecart Speed Plus");
            modalForm.setContent(
                fmt::format(" 您购买的套餐价格: {} \n 绑定矿车ID: {} \n 是否确定购买", data.money, actorUniqueId.id)
            );
            modalForm.setUpperButton("确定");
            modalForm.setLowerButton("取消");
            modalForm.sendTo(
                player,
                [&](Player&                                     player,
                    optional<ll::form::ModalFormSelectedButton> Button,
                    optional<ll::form::FormCancelReason>) {
                    if (Button.has_value()) {
                        if (Button) {
                            Scoreboard& scoreboard = ll::service::getLevel()->getScoreboard();
                            Objective*  objective  = scoreboard.getObjective(config.ScoreName);
                            if (objective == nullptr) {
                                my_mod::MyMod::getInstance().getSelf().getLogger().error(
                                    "Objective {} not found",
                                    config.ScoreName
                                );
                                return;
                            };
                            const ScoreboardId& id = scoreboard.getScoreboardId(player);
                            if (!id.isValid()) {
                                scoreboard.createScoreboardId(player);
                            }
                            auto score = objective->getPlayerScore(id).mScore;
                            if (score < data.money) {
                                player.sendMessage("[MinecartSpeedPlus] 您的积分不够");
                                return;
                            }
                            score    -= data.money;
                            bool res  = false;
                            scoreboard.modifyPlayerScore(res, id, *objective, score, PlayerScoreSetFunction::Set);
                            if (!res) return;
                            minecartDB->set(std::to_string(actorUniqueId.id), std::to_string(data.speed));
                            Actor* minecart = ll::service::getLevel()->fetchEntity(actorUniqueId);
                            if (minecart == nullptr) {
                                minecartDB->del(std::to_string(actorUniqueId.id));
                                return;
                            }
                            if (minecart->hasTag("minecartSpeedPlus")) {
                                player.sendMessage("[MinecartSpeedPlus] 您已购买过套餐 不可重新购买或者更换");
                                return;
                            } else {
                                minecart->addTag("minecartSpeedPlus");
                                player.sendMessage("[MinecartSpeedPlus] 购买成功");
                            }
                        }
                    }
                }
            );
        });
    });
    form.appendButton("回收矿车", [&](Player& player) {
        Actor* minecart = ll::service::getLevel()->fetchEntity(actorUniqueId);
        if (minecart == nullptr) {
            minecartDB->del(std::to_string(actorUniqueId.id));
            player.sendMessage("[MinecartSpeedPlus] 矿车不存在");
            return;
        }
        minecart->kill();
        player.sendMessage("[MinecartSpeedPlus] 成功回收矿车");
        if (player.hasTag("alreadyAttackMineCart")) {
            player.removeTag("alreadyAttackMineCart");
        }
        if (player.hasTag("destoryminecart")) {
            player.removeTag("destoryminecart");
        }
    });
    form.sendTo(player);
}

void RegListener() {
    auto& eventBus = ll::event::EventBus::getInstance();
    eventBus.emplaceListener<ll::event::PlayerAttackEvent>([&](ll::event::PlayerAttackEvent& ev) {
        auto& player = ev.self();
        auto& target = ev.target();
        if (target.getTypeName() == "minecraft:minecart") {
            auto MineCartPos  = target.getPosition();
            MineCartPos.y    -= 0.2f;
            auto& DownType    = target.getDimensionBlockSource().getBlock(target.getPosition()).getTypeName();
            my_mod::MyMod::getInstance().getSelf().getLogger().debug(DownType);
            if (DownType != "minecraft:golden_rail") {
                if (player.hasTag("destoryminecart") == false) {
                    player.sendMessage("[MinecartSpeedPlus] 只能在充能铁轨上打开表单 再攻击一次将破坏矿车");
                    player.addTag("destoryminecart");
                    target.addTag("destoty");
                    return;
                } else if (player.hasTag("destoryminecart") && target.hasTag("destoty")
                           && DownType != "minecraft:golden_rail") {
                    target.removeTag("destoty");
                    player.removeTag("destoryminecart");
                    target.kill();
                    player.sendMessage("[MinecartSpeedPlus] 成功破坏矿车");
                    return;
                }
            } else {
                if (player.hasTag("alreadyAttackMineCart")) {
                    Form(player, target.getOrCreateUniqueID());
                    player.removeTag("alreadyAttackMineCart");
                    return;
                } else {
                    player.addTag("alreadyAttackMineCart");
                    player.sendMessage("[MinecartSpeedPlus] 再攻击一次打开速度更改购买表单");
                    return;
                }
            }
            return;
        }
    });
}

// LL_AUTO_TYPE_STATIC_HOOK(
//     calculateGoldenRailSpeedIncreaseHook,
//     HookPriority::Normal,
//     RailMovementUtility,
//     &RailMovementUtility::calculateGoldenRailSpeedIncrease,
//     Vec3,
//     class IConstBlockSource const& region,
//     class BlockPos const&          pos,
//     int                            direction,
//     class Vec3                     posDelta
// ) {
//     auto res = origin(region, pos, direction, posDelta);
//     my_mod::MyMod::getInstance().getSelf().getLogger().info("Hook");
//
//     return res;
// }

LL_AUTO_TYPE_STATIC_HOOK(
    calculateMoveVelocityHook,
    HookPriority::Normal,
    RailMovementUtility,
    &RailMovementUtility::calculateMoveVelocity,
    Vec3,
    class Block const&                            a,
    int                                           b,
    float                                         c,
    bool                                          d,
    class Vec3&                                   e,
    bool&                                         f,
    bool&                                         g,
    class std::function<bool(class Vec3&)> const& h
) {
    auto res = origin(a, b, c, d, e, f, g, h);
    // my_mod::MyMod::getInstance().getSelf().getLogger().info("{} {} {}", d, f, g);
    if (a.getTypeName() == "minecraft:golden_rail") {
        if (f == true) return res;
        minecartDB->iter([&](const std::string_view& key, const std::string_view& value) {
            auto   UniqueID = ActorUniqueID(std::stoll(std::string(key)));
            Actor* minecart = ll::service::getLevel()->fetchEntity(UniqueID);
            if (minecart == nullptr) {
                minecartDB->del(key);
                return true;
            }
            if (minecart->hasTag("minecartSpeedPlus")) {
                res *= std::stoi(std::string(value));
            }
            return true;
        });
    }
    return res;
}


LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerAttackBlockHook,
    HookPriority::Normal,
    Block,
    &Block::attack,
    bool,
    Player*         player,
    BlockPos const& pos
) {
    if (player->hasTag("alreadyAttackMineCart")) {
        player->removeTag("alreadyAttackMineCart");
    }
    if (player->hasTag("destoryminecart")) {
        player->removeTag("destoryminecart");
    }
    auto res = origin(player, pos);
    return res;
}