// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2021 YADRO

#ifndef __ENTITY_H__
#define __ENTITY_H__

#include <core/exceptions.hpp>
#include <logger/logger.hpp>

#include <definitions.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace app
{
namespace entity
{

class IEntity;
class IEntityMapper;
class EntityManager;
class EntitySupplementProvider;

using EntityPtr = std::shared_ptr<IEntity>;
using EntityPtrConst = std::shared_ptr<const IEntity>;
using EntityWeak = std::weak_ptr<IEntity>;
using EntityManagerPtr = std::shared_ptr<EntityManager>;
using EntityManagerUni = std::unique_ptr<EntityManager>;
using EntitySupplementProviderPtr = std::shared_ptr<EntitySupplementProvider>;

using EntityName = std::string;
using MemberName = std::string;

namespace exceptions
{

class EntityException : public core::exceptions::ObmcAppException
{
  public:
    explicit EntityException(const std::string& arg) :
        core::exceptions::ObmcAppException(arg)
    {}
    virtual ~EntityException() noexcept = default;
};

} // namespace exceptions

class IEntity
{
  public:
    class IEntityMember;
    class IInstance;
    class ISupplementProvider;
    class IRelation;
    class ICondition;

    using EntityMemberPtr = std::shared_ptr<IEntity::IEntityMember>;
    using EntityMemberPtrConst = std::shared_ptr<const IEntity::IEntityMember>;
    using InstancePtr = std::shared_ptr<IInstance>;
    using InstancePtrConst = std::shared_ptr<const IInstance>;
    using RelationPtr = std::shared_ptr<IRelation>;
    using ConditionPtr = std::shared_ptr<ICondition>;

    using MemberMap = std::map<const std::string, EntityMemberPtr>;

    class IEntityMember
    {
      public:
        class IInstance;
        using InstancePtr = std::shared_ptr<IInstance>;
        using InstancePtrConst = std::shared_ptr<const IInstance>;

        class IInstance
        {
          public:
            using FieldType =
                std::variant<std::string, int64_t,
                             uint64_t, double, int32_t, uint32_t, int16_t,
                             uint16_t, uint8_t, bool>;

            virtual const FieldType& getValue() const noexcept = 0;
            virtual const std::string& getStringValue() const = 0;
            virtual int getIntValue() const = 0;
            virtual double getFloatValue() const = 0;
            virtual bool getBoolValue() const = 0;

            virtual void setValue(const FieldType&) = 0;

            virtual ~IInstance() noexcept = default;

            friend std::ostream& operator<<(std::ostream& os,
                                            const FieldType& variantValue)
            {
                std::visit(
                    [&os](auto&& value) {
                        using TProperty = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<TProperty, std::string> ||
                                      std::is_arithmetic_v<TProperty>)
                        {
                            os << value;
                        }
                        else if constexpr (std::is_same_v<TProperty, bool>)
                        {
                            os << std::boolalpha << value;
                        }
                    },
                    variantValue);
                return os;
            }
        };

        virtual const std::string getName() const noexcept = 0;
        virtual const InstancePtr& getInstance() const = 0;

        virtual ~IEntityMember() noexcept = default;
    };

    class IInstance
    {
      public:
        virtual ~IInstance() noexcept = default;
        /**
         * @brief Get the Field of Entity Instance
         *
         * @param entityMemberName - name of an Entity Member to seach the Field
         *
         * @return const Entity::IEntityMember::InstancePtr& The Field Instance
         * of specified Entity Member
         *
         */
        virtual const IEntity::IEntityMember::InstancePtr&
            getField(const IEntity::EntityMemberPtr& entityMember) const = 0;

        virtual const IEntity::IEntityMember::InstancePtr&
            getField(const MemberName& entityMemberName) const = 0;

        virtual void supplement(const MemberName&,
                                const IEntityMember::IInstance::FieldType&) = 0;
        virtual void supplementOrUpdate(const MemberName&,
                                const IEntityMember::IInstance::FieldType&) = 0;

        virtual bool hasField(const MemberName& ) const = 0;
        virtual bool checkCondition(const ConditionPtr) const = 0;

        virtual const std::map<std::size_t, InstancePtr> getComplex() const = 0;
        virtual bool isComplex() const = 0;

        virtual void initDefaultFieldsValue() = 0;
        /**
         * @brief Get the Hash of Entity Instance
         *
         * @return std::size_t hash value
         */
        virtual std::size_t getHash() const = 0;
    };

    class ISupplementProvider
    {
      public:
        using ProviderLinkRule = std::function<void(const IEntity::InstancePtr&,
                                                    const IEntity::InstancePtr&)>;

        virtual ~ISupplementProvider() noexcept = default;

        virtual void supplementInstance(IEntity::InstancePtr&,
                                        ProviderLinkRule) = 0;
    };

    class ICondition
    {
      public:
        using CompareCallback =
            std::function<bool(const IEntityMember::InstancePtr&,
                               const IEntityMember::IInstance::FieldType&)>;

        virtual void addRule(const MemberName&,
                             const IEntityMember::IInstance::FieldType&,
                             CompareCallback) = 0;

        virtual bool check(const IEntity::IInstance&) const = 0;

        virtual ~ICondition() noexcept = default;
    };

    class IRelation
    {
      public:
        using RuleSet =
            std::tuple<MemberName, MemberName, ICondition::CompareCallback>;
        using RelationRulesList = std::vector<RuleSet>;

        enum class LinkWay : uint8_t
        {
            oneToOne,
            oneToMany,
            manyToOne,
            none
        };
        virtual ~IRelation() noexcept = default;

        virtual const EntityPtr& getDestinationTarget() const = 0;
        virtual const std::vector<ConditionPtr> getConditions() const = 0;

        virtual LinkWay getLinkWay() const = 0;
    };

    virtual const EntityName getName() const noexcept = 0;

    virtual bool addMember(const EntityMemberPtr&) = 0;

    virtual const IEntity::EntityMemberPtr
        getMember(const std::string& memberName) const = 0;

    virtual const MemberMap& getMembers() const = 0;

    virtual const InstancePtr getInstance(std::size_t) const = 0;
    virtual const std::vector<InstancePtr>
        getInstances(const ConditionPtr = ConditionPtr()) const = 0;
    virtual void setInstances(std::vector<InstancePtr>) = 0;

    virtual void
        linkSupplementProvider(const EntitySupplementProviderPtr&,
                               ISupplementProvider::ProviderLinkRule) = 0;

    virtual void addRelation(const RelationPtr) = 0;
    virtual const std::vector<RelationPtr>& getRelations() const = 0;

    virtual ~IEntity() noexcept = default;
};

class Entity : public IEntity
{
    using ProviderRulesDict =
        std::vector<std::pair<const EntitySupplementProviderPtr,
                              ISupplementProvider::ProviderLinkRule>>;
    using InstanceHash = std::size_t;
    MemberMap members;
    const EntityName name;
    std::map<InstanceHash, InstancePtr> instances;
    ProviderRulesDict providers;
    std::vector<RelationPtr> relations;

  public:
    class EntityMember : public IEntityMember
    {
        const MemberName name;
        InstancePtr instance;

      public:
        static constexpr const char* fieldValueNotAvailable = "N/A";
        class StaticInstance : public IEntityMember::IInstance
        {
            FieldType value;

          public:
            StaticInstance(const StaticInstance&) = delete;
            StaticInstance& operator=(const StaticInstance&) = delete;
            StaticInstance(StaticInstance&&) = delete;
            StaticInstance& operator=(StaticInstance&&) = delete;

            explicit StaticInstance(const FieldType& fieldValue) noexcept :
                value(fieldValue)
            {}
            ~StaticInstance() noexcept override = default;

            const FieldType& getValue() const noexcept
            {
                return value;
            }
            void setValue(const FieldType& fieldValue)
            {
                value = fieldValue;
            }
            const std::string& getStringValue() const override
            {
                return std::get<std::string>(value);
            }
            int getIntValue() const override
            {
                return std::get<int>(value);
            }
            double getFloatValue() const override
            {
                return std::get<double>(value);
            }
            bool getBoolValue() const override
            {
                return std::get<bool>(value);
            }
        };

        explicit EntityMember(const std::string& memberName) noexcept :
            name(memberName), instance(std::make_shared<StaticInstance>(
                                  std::string(fieldValueNotAvailable)))
        {}

        explicit EntityMember() = delete;
        EntityMember(const EntityMember&) = delete;
        EntityMember& operator=(const EntityMember&) = delete;
        EntityMember(EntityMember&&) = delete;
        EntityMember& operator=(EntityMember&&) = delete;

        ~EntityMember() noexcept override = default;

        const MemberName getName() const noexcept override;

        const InstancePtr& getInstance() const override;
    };

    class Condition : public ICondition
    {
        using RuleMeta = std::pair<MemberName, IEntityMember::IInstance::FieldType>;
        using Rule = std::pair<RuleMeta, CompareCallback>;
        std::vector<Rule> rules;
      public:
        Condition(const Condition&) = delete;
        Condition& operator=(const Condition&) = delete;
        Condition(Condition&&) = delete;
        Condition& operator=(Condition&&) = delete;

        explicit Condition() noexcept =default;
        ~Condition() noexcept override = default;

        void addRule(const MemberName&,
                     const IEntityMember::IInstance::FieldType&,
                     CompareCallback) override;

        bool check(const IEntity::IInstance&) const override;
      protected:
        bool fieldValueCompare(const IEntity::IInstance&) const;
    };

    class Relation : public IRelation
    {
        const EntityWeak source;
        const EntityPtr destination;
        LinkWay linkWay;
        RelationRulesList conditionBuildRules;
      public:
        Relation(const Relation&) = delete;
        Relation& operator=(const Relation&) = delete;
        Relation(Relation&&) = delete;
        Relation& operator=(Relation&&) = delete;

        explicit Relation(const EntityWeak sourceEntity,
                          const EntityPtr destinationEntity) noexcept :
            source(sourceEntity),
            destination(destinationEntity), linkWay(LinkWay::oneToOne)
        {}
        ~Relation() noexcept override = default;

        void addConditionBuildRules(const RelationRulesList&);

        const EntityPtr& getDestinationTarget() const override;
        const std::vector<ConditionPtr> getConditions() const override;
        LinkWay getLinkWay() const override;
    };


    explicit Entity() = delete;
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;

    explicit Entity(const std::string& objectName) noexcept : name(objectName)
    {
    }

    ~Entity() noexcept override = default;

    bool addMember(const EntityMemberPtr&) override;
    const std::string getName() const noexcept override;

    const IEntity::EntityMemberPtr
        getMember(const std::string& memberName) const override;

    const MemberMap& getMembers() const override
    {
        return this->members;
    }
    const InstancePtr getInstance(std::size_t) const override;
    const std::vector<InstancePtr>
        getInstances(const ConditionPtr = ConditionPtr()) const override;
    void setInstances(std::vector<InstancePtr>) override;

    void linkSupplementProvider(const EntitySupplementProviderPtr&,
                                ISupplementProvider::ProviderLinkRule) override;

    void addRelation(const RelationPtr) override;
    const std::vector<RelationPtr>& getRelations() const override;
};

class EntitySupplementProvider :
    public Entity,
    public IEntity::ISupplementProvider
{
  public:
    explicit EntitySupplementProvider(const std::string& providerName) noexcept
        :
        Entity(providerName)
    {}
    ~EntitySupplementProvider() noexcept override = default;

    void supplementInstance(IEntity::InstancePtr& instance,
                            ProviderLinkRule) override;
};

class EntityManager final
{
    using EntityMap = std::map<const std::string, EntityPtr>;
    using SupplementProviderDict =
        std::map<std::string, EntitySupplementProviderPtr>;

  public:
    class EntityBuilder;

    using EntityBuilderPtr = std::shared_ptr<EntityBuilder>;
    using EntityBuilderUni = std::unique_ptr<EntityBuilder>;

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;
    EntityManager(EntityManager&&) = delete;
    EntityManager& operator=(EntityManager&&) = delete;

    explicit EntityManager() = default;
    ~EntityManager() = default;

    class EntityBuilder final :
        public std::enable_shared_from_this<EntityBuilder>
    {
        EntityPtr entity;
        const SupplementProviderDict& providers;
        EntityManager& entityManager;
      public:
        EntityBuilder(const EntityBuilder&) = delete;
        EntityBuilder& operator=(const EntityBuilder&) = delete;
        EntityBuilder(EntityBuilder&&) = delete;
        EntityBuilder& operator=(EntityBuilder&&) = delete;

        explicit EntityBuilder(
            EntityPtr targetEntity,
            const SupplementProviderDict& supplementProviders,
            EntityManager& entityManagerRef) :
            entity(targetEntity),
            providers(supplementProviders), entityManager(entityManagerRef)
        {}
        ~EntityBuilder() noexcept = default;

        template <class TQueryBuilder, typename TBrokerManager>
        std::shared_ptr<TQueryBuilder> addQuery(TBrokerManager& manager)
        {
            return std::make_shared<TQueryBuilder>(shared_from_this(), entity,
                                                   manager);
        }

        EntityBuilder& addMembers(const std::vector<std::string>& memberNames);

        EntityBuilder& linkSupplementProvider(
            const std::string& providerName,
            IEntity::ISupplementProvider::ProviderLinkRule);

        EntityBuilder& addRelations(const std::string&,
                                    const IEntity::IRelation::RelationRulesList&);
    };

    EntityBuilderPtr buildSupplementProvider(const std::string&);

    EntityBuilderPtr buildEntity(const std::string&);

    void addEntity(EntityPtr entity);

    /**
     * @brief Get the Objects object
     *
     * @param entityName - the entity name to retrieve the corresponding an
     * Entity Objects instances
     * @return const EntityPtr - Entity object which contains the target
     * instances
     */
    const EntityPtr getEntity(const EntityName& entityName) const;

  protected:
    EntityMap entityDictionary;
    SupplementProviderDict supplementProviders;
};

/**
 * @brief The class of mapping the actual Entity literal name with the protocol
 * specific naming.
 *
 * TODO: by actual this abstraction unimplemented. But for the future protocols
 * implementation, e.g. DMTF REDFISH, this is must have.
 *
 * The Entity instances do registered as:
 * + `Entity object literal name` => EntityObject instance
 *    - `Entity Object Member literal name` => EntityObjectMember instance
 *    - ...
 * + ...
 *
 * A some protocols may want to extract the EntityObject/EntityMember by own
 * specific literal naming.
 * The IEntityMapper mappings the requested literal
 * name to the registered literal name.
 *
 */
class IEntityMapper
{
  public:
    virtual ~IEntityMapper() noexcept = default;
};

} // namespace entity
} // namespace app

#endif // __ENTITY_H__
