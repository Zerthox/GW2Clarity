﻿#pragma once
#include <ConfigurationFile.h>
#include <Main.h>
#include <MumbleLink.h>
#include <list>
#include <charconv>
#include <functional>
#include <numeric>
#include <optional>

namespace GW2Radial
{

class Condition {
protected:
    uint id_ = 0;
    bool negate_ = false;
    [[nodiscard]] virtual bool test() const = 0;

    std::string paramName(const char* param) const {
        return "condition_" + std::to_string(id_) + "_" + nickname() + "_" + param;
    }

public:
    explicit Condition(uint id) : id_(id) {}
    virtual ~Condition() = default;
    
    [[nodiscard]] virtual std::string nickname() const = 0;

    [[nodiscard]] uint id() const { return id_; }

    [[nodiscard]] bool negate() const { return negate_; }
    Condition& negate(bool negate) { negate_ = negate; return *this; }

    [[nodiscard]] bool passes() const { return test() != negate_; }
    
    virtual void Save(const char* category) const {
        ConfigurationFile::i()->ini().SetBoolValue(category, paramName("negate").c_str(), negate_);
    }
    virtual void Load(const char* category) {
        negate_ = ConfigurationFile::i()->ini().GetBoolValue(category, paramName("negate").c_str(), false);
    }
};

class IsInCombatCondition final : public Condition {
    using Condition::Condition;

    [[nodiscard]] bool test() const override;
    [[nodiscard]] std::string nickname() const override { return "in_combat"; }
};

class IsWvWCondition final : public Condition {
    using Condition::Condition;

    [[nodiscard]] bool test() const override;
    [[nodiscard]] std::string nickname() const override { return "wvw"; }
};

class IsUnderwaterCondition final : public Condition {
    using Condition::Condition;

    [[nodiscard]] bool test() const override;
    [[nodiscard]] std::string nickname() const override { return "underwater"; }
};

class IsProfessionCondition final : public Condition {
    using Condition::Condition;
    MumbleLink::Profession profession_;

    [[nodiscard]] bool test() const override;
    [[nodiscard]] std::string nickname() const override { return "profession"; }

public:
    [[nodiscard]] MumbleLink::Profession profession() const { return profession_; }
    void profession(MumbleLink::Profession id) { profession_ = id; }
    [[nodiscard]] bool operator==(const IsProfessionCondition& other) const;

    void Save(const char* category) const override {
        Condition::Save(category);
        ConfigurationFile::i()->ini().SetLongValue(category, paramName("id").c_str(), static_cast<long>(profession_));
    }
    void Load(const char* category) override {
        Condition::Load(category);
        profession_ = static_cast<MumbleLink::Profession>(ConfigurationFile::i()->ini().GetLongValue(category, paramName("id").c_str(), 0));
    }
};

class IsCharacterCondition final : public Condition {
    using Condition::Condition;
    std::wstring characterName_;
    
    [[nodiscard]] bool test() const override;
    [[nodiscard]] std::string nickname() const override { return "character"; }

public:
    [[nodiscard]] const std::wstring& characterName() const { return characterName_; }
    [[nodiscard]] bool operator==(const IsCharacterCondition& other) const;

    void Save(const char* category) const override {
        Condition::Save(category);
        ConfigurationFile::i()->ini().SetValue(category, paramName("charname").c_str(), utf8_encode(characterName_).c_str());
    }
    void Load(const char* category) override {
        Condition::Load(category);
        characterName_ = utf8_decode(ConfigurationFile::i()->ini().GetValue(category, paramName("charname").c_str(), ""));
    }
};

enum class ConditionOp {
    OR = 0,
    AND = 1,
    OPEN_PAREN = 2,
    CLOSE_PAREN = 4
};

inline ConditionOp operator|(const ConditionOp& a, const ConditionOp& b) {
    return ConditionOp(uint(a) | uint(b));
}

class ConditionSet {
    std::string category_;

    struct ConditionEntry {
        std::unique_ptr<Condition> condition;
        ConditionOp prevOp = ConditionOp::OR;
    };

    std::list<ConditionEntry> conditions_;

    void Load();
    bool DrawBaseMenuItem(Condition& c, const char* enableDesc, const char* disableDesc, std::optional<std::function<bool()>> extras = std::nullopt);
public:
    explicit ConditionSet(std::string category);

    [[nodiscard]] bool passes() const;
    [[nodiscard]] bool conflicts(const ConditionSet* other) const;
    void Save() const;
    void DrawMenu();
};
using ConditionSetPtr = std::shared_ptr<ConditionSet>;

}
