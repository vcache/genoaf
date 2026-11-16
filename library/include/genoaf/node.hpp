#pragma once

#include <genoaf/errors.hpp>
#include <genoaf/misc.hpp>
#include <genoaf/property.hpp>

#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace genoaf
{
    class InnerNode;
    class Node;
    class SceneTree;

    // SceneTree, a class holding root and track changes

    class SceneTree
    {
        GENOAF_NO_COPY(SceneTree);
        GENOAF_NO_MOVE(SceneTree);

    public:
        SceneTree();
        ~SceneTree();

        InnerNode & root() { assert(_root); return *_root; }
        InnerNode const & root() const { assert(_root); return *_root; }

    public:
        enum class Pass
        {
            kStructure  = 0,
            kProperties = 1,
            kAnimation  = 2,
            kOrigins    = 3,
            kTransforms = 4,
            kOutlines   = 5,
            kVisibility = 6,

            kCount // Must be the last one
        };

        constexpr static size_t kPassSize = static_cast<size_t>(Pass::kCount);

        // Function signature is the following:
        //      bool(Node &)
        //
        // It must return true if the node has been
        // revalidated, and it can be removed from dirty-list.
        template<typename Function>
        void revalidate(Pass pass, Function function);

        // Must be called after all revalidation passes
        void finish();

    private:
        void markDirty(Node & node, Pass pass);
        void changeLevel(Node & node, size_t oldLevel, size_t newLevel);
        void ensureSize(size_t requiredLevel);

    private:
        // The structure:
        //  Level 0
        //   ...
        //  Level i
        //      DirtyNodes (Pass::kAnimation)
        //      DirtyNodes (Pass::kOrigins)
        //       ...
        //   ...
        //  Level N-1

        using DirtyNodes = std::vector<Node *>;
        using DirtyLevel = std::array<DirtyNodes, kPassSize>;

        std::shared_ptr<InnerNode> _root;
        std::vector<DirtyLevel>    _dirtyLevels;
        size_t                     _minDirtyLevel = 0;
        size_t                     _maxDirtyLevel = 0;

        friend class Node;
    };

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
        explicit Node(SceneTree & sceneTree, std::string id)
            : _sceneTree(sceneTree)
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
            markDirty(SceneTree::Pass::kProperties);
        }

        void markDirty(SceneTree::Pass const pass)
        {
            if (_level != kNoIndex)
                _sceneTree.markDirty(*this, pass);
        }

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
        using LevelIndeces = std::array<size_t, SceneTree::kPassSize>;

        SceneTree                   & _sceneTree;
        std::string const             _id;
        size_t                        _level = kNoIndex;
        LevelIndeces                  _levelIndeces;
        Property<InnerNodeWptr, true> _parent;
        Property<bool>                _visible;

        friend class InnerNode;
        friend class SceneTree;
    };

    // InnerNode, a node with children

    class InnerNode
        : public Node
    {
        GENOAF_NO_COPY(InnerNode);
        GENOAF_NO_MOVE(InnerNode);

    public:
        GENOAF_SPTR(InnerNode);

        explicit InnerNode(SceneTree & sceneTree, std::string id)
            : Node(sceneTree, std::move(id))
            , _children(*this)
        {}

        ~InnerNode() override;

    public:
        bool empty() const { return _children.get().empty(); }
        size_t size() const { return _children.get().size(); }

        bool contains(std::string const & id) const { return _children.get().contains(id); }
        bool contains(Path const & path) const { return find(path).operator bool(); }

    public:
        // NOTE: since find() functions return std::shared_ptr,
        //       the caller must be careful storing it inside non-automatic
        //       variables. For long-term storage it must be converted to Wptr.

        Node::CSptr find(std::string const & id) const;
        Node::CSptr find(Path const & path) const;

        template<typename T>
        std::shared_ptr<T const> find(std::string const & id) const
        {
            return std::dynamic_pointer_cast<T const>(find(id));
        }

        template<typename T>
        std::shared_ptr<T const> find(Path const & path) const
        {
            return std::dynamic_pointer_cast<T const>(find(path));
        }

        template<typename T>
        T const & at(std::string const & id) const
        {
            auto it = _children.get().find(id);
            GENOAF_INVARIANT(it != _children.get().cend(),
                             "no such node in \"{}\": \"{}\"", absPath(), id);
            T const * result = dynamic_cast<T const *>(it->second.get());
            GENOAF_INVARIANT(result, "failed to cast \"{}\" (in \"{}\") to {} ({})",
                             id, absPath(), demangle<T>(), it->second ? "found" : "not found");
            return *result;
        }

        template<typename T>
        T const & at(Path const & path) const
        {
            Node::CSptr const & node = find(path);
            GENOAF_INVARIANT(node, "no such path in \"{}\": \"{}\"", absPath(), path);
            T const * result = dynamic_cast<T const *>(node.get());
            GENOAF_INVARIANT(result, "failed to cast \"{}\" to {}", path, demangle<T>());
            return *result;
        }

    public:
        Node::Sptr find(std::string const & id)
        {
            Node::CSptr const & result = const_cast<InnerNode const *>(this)->find(id);
            return std::const_pointer_cast<Node>(result);
        }

        Node::Sptr find(Path const & path)
        {
            Node::CSptr const & result = const_cast<InnerNode const *>(this)->find(path);
            return std::const_pointer_cast<Node>(result);
        }

        template<typename T>
        std::shared_ptr<T> find(std::string const & id)
        {
            std::shared_ptr<T const> result = const_cast<InnerNode const *>(this)->find<T>(id);
            return std::const_pointer_cast<T>(result);
        }

        template<typename T>
        std::shared_ptr<T> find(Path const & path)
        {
            std::shared_ptr<T const> result = const_cast<InnerNode const *>(this)->find<T>(path);
            return std::const_pointer_cast<T>(result);
        }

        template<typename T>
        T & at(std::string const & id)
        {
            T const & result = const_cast<InnerNode const *>(this)->at<T>(id);
            return const_cast<T &>(result);
        }

        template<typename T>
        T & at(Path const & path)
        {
            T const & result = const_cast<InnerNode const *>(this)->at<T>(path);
            return const_cast<T &>(result);
        }

    public:
        // NOTE: will automatically connect to SceneTree.
        // NOTE: the first parameter is new node's ID (it is mandatory).
        // NOTE: user should avoid storing Sptr, because this can undesirably
        //       extend node's lifetime. Long-term storing should be done w/ Wptr.
        // NOTE: "T &" may become dangling if emplaced node has been deleted, so,
        //       avoid storing it as reference (unless trivial cases).
        // NOTE: use weak_from_this (the safest) or shared_from_this to fetch smart ptr.
        template<typename T, typename... Args>
        T & emplace(std::string id, Args && ... args)
        {
            auto result = std::make_shared<T>(_sceneTree, id,
                                              std::forward<Args>(args)...);
            insert(*result);
            return *result;
        }

        void insert(Node::Sptr const & node) { if (node) insert(*node); }

        void insert(Node & node)
        {
            node.setParent(std::static_pointer_cast<InnerNode>(shared_from_this()));
        }

        void erase(std::string const & id)
        {
            if (auto it = _children.get().find(id);
                it != _children.get().cend() && it->second)
            {
                it->second->setParent(InnerNode::Sptr{});
            }
        }

        void clear();

    public:
        auto begin() { return _children->begin(); }
        auto end() { return _children->end(); }

        auto begin() const { return _children.get().cbegin(); }
        auto end() const { return _children.get().end(); }

    public:
        InnerNode const * asInnerNode() const override { return this; }
        InnerNode * asInnerNode() override { return this; }

        std::shared_ptr<InnerNode const> asInnerNodeSptr() const override
        {
            return std::static_pointer_cast<InnerNode const>(shared_from_this());
        }

        std::shared_ptr<InnerNode> asInnerNodeSptr() override
        {
            return std::static_pointer_cast<InnerNode>(shared_from_this());
        }

    private:
        void attach(Node & node);
        void detach(Node const & node);

        bool updateLevel() override;

    private:
        using Children = std::unordered_map<std::string, Node::Sptr>;

        Property<Children> _children;

        friend class Node;
    };

    // Inline implementations

    template<typename Function>
    void SceneTree::revalidate(Pass pass, Function function)
    {
        size_t const passIndex = static_cast<size_t>(pass);
        for(size_t level = _minDirtyLevel; level <= _maxDirtyLevel; ++level)
        {
            assert(level < _dirtyLevels.size());
            DirtyLevel & dirtyLevel = _dirtyLevels[level];

            assert(passIndex < dirtyLevel.size());
            DirtyNodes & dirtyNodes = dirtyLevel[passIndex];

            for(size_t i = 0; i < dirtyNodes.size();)
            {
                assert(dirtyNodes[i]);
                Node & current = *dirtyNodes[i];
                if (function(current))
                {
                    assert(i < dirtyNodes.size());
                    assert(dirtyNodes[i]->_levelIndeces[passIndex] == i);
                    dirtyNodes[i] = dirtyNodes.back();

                    assert(dirtyNodes[i]);
                    assert(dirtyNodes[i]->_levelIndeces[passIndex] == dirtyNodes.size() - 1);
                    dirtyNodes[i]->_levelIndeces[passIndex] = i;
                    dirtyNodes.pop_back();

                    current._levelIndeces[passIndex] = kNoIndex;
                }
                else
                {
                    ++i;
                }
            }
        }
    }
}