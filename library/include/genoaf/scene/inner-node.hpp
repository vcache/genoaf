#pragma once

#include <genoaf/errors.hpp>
#include <genoaf/misc.hpp>
#include <genoaf/property.hpp>
#include <genoaf/scene/node.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace genoaf::scene
{
    // InnerNode, a node with children
    class InnerNode
        : public Node
    {
        GENOAF_NO_COPY(InnerNode);
        GENOAF_NO_MOVE(InnerNode);

    public:
        GENOAF_SPTR(InnerNode);

        explicit InnerNode(Tree & tree, std::string id)
            : Node(tree, std::move(id))
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
        // NOTE: will automatically connect to Tree.
        // NOTE: the first parameter is new node's ID (it is mandatory).
        // NOTE: user should avoid storing Sptr, because this can undesirably
        //       extend node's lifetime. Long-term storing should be done w/ Wptr.
        // NOTE: "T &" may become dangling if emplaced node has been deleted, so,
        //       avoid storing it as reference (unless trivial cases).
        // NOTE: use weak_from_this (the safest) or shared_from_this to fetch smart ptr.
        template<typename T, typename... Args>
        T & emplace(std::string id, Args && ... args)
        {
            auto result = std::make_shared<T>(_tree, id,
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
        bool updateLevel() override;

    private:
        using Children = std::unordered_map<std::string, Node::Sptr>;

        Property<Children> _children;

        friend class Node;
    };
}