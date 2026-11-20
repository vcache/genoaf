#pragma once

#include <genoaf/misc.hpp>
#include <genoaf/property.hpp>
#include <genoaf/scene/revalidation-pass.hpp>

#include <array>
#include <memory>
#include <string>

namespace genoaf::scene
{
    class InnerNode;
    class Tree;

    // Node, a node without children
    class Node
        : public std::enable_shared_from_this<Node>
    {
        GENOAF_NO_COPY(Node);
        GENOAF_NO_MOVE(Node);

    public:
        GENOAF_SPTR(Node);

        template<typename T, bool kSkipEqTest = false>
        using Property = Property<T, Node, kSkipEqTest>;

    public:
        explicit Node(Tree & tree, std::string id)
            : _tree(tree)
            , _id(std::move(id))
            , _level(kNoIndex)
            , _parent(*this)
            , _visible(*this)
        {
            _levelIndeces.fill(kNoIndex);
        }

        virtual ~Node();

        std::string const & id() const { return _id; }

        size_t level() const { return _level; }

        Path absPath() const;

    public:
        template<typename Type,
                 bool kSkipEqTest>
        void markDirty(::genoaf::Property<Type, Node, kSkipEqTest> const &)
        {
            markDirty(RevalidationPass::kProperties);
        }

        void markDirty(RevalidationPass const pass);

    public:
        std::weak_ptr<InnerNode const> wparent() const { return _parent.get(); }
        std::weak_ptr<InnerNode> wparent() { return _parent.get(); }

        std::shared_ptr<InnerNode const> parent() const { return wparent().lock(); }
        std::shared_ptr<InnerNode> parent() { return wparent().lock(); }

        void setParent(std::shared_ptr<InnerNode> const & newParent);
        void setParent(InnerNode & newParent);

    public:
        // ligthweight alternative to dynamic_cast
        virtual InnerNode const * asInnerNode() const { return nullptr; }
        virtual InnerNode * asInnerNode() { return nullptr; }
        virtual std::shared_ptr<InnerNode const> asInnerNodeSptr() const { return {}; }
        virtual std::shared_ptr<InnerNode> asInnerNodeSptr() { return {}; }

    protected:
        virtual bool updateLevel();

    private:
        using InnerNodeWptr = std::weak_ptr<InnerNode>;
        using LevelIndeces = std::array<size_t, kRevalidationPassCount>;

        Tree                        & _tree;
        std::string const             _id;
        size_t                        _level = kNoIndex;
        LevelIndeces                  _levelIndeces;
        Property<InnerNodeWptr, true> _parent;
        Property<bool>                _visible;

        friend class InnerNode;
        friend class Tree;
    };
}