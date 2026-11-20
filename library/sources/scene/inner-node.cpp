#include <genoaf/scene/inner-node.hpp>

#include <genoaf/errors.hpp>
#include <genoaf/scene/tree.hpp>

#include <algorithm>
#include <cassert>

namespace genoaf::scene
{
    InnerNode::~InnerNode()
    {
        // children must be erased manually to ensure
        // that they are recursively un-parented and, thus,
        // dirty lists are cleared properly
        clear();
    }

    void InnerNode::clear()
    {
        // NOTE: since it may be called from dtor,
        //       children won't be able to call detach(),
        //       because shared_ptr will be already expired.

        auto children = _children.get();
        for (auto const & [_, child] : children)
        {
            assert(child);
            if (InnerNode * childAsInner = child->asInnerNode();
                childAsInner)
            {
                childAsInner->clear();
                assert(childAsInner->empty());
            }

            // NOTE: it MAY erase itself from _children (via detach),
            //       unless it is called from dtor
            child->setParent({});
        }

        _children->clear();
    }

    Node::CSptr InnerNode::find(std::string const & id) const
    {
        auto it = _children.get().find(id);
        return it != _children.get().cend() ? it->second
                                            : Node::CSptr{};
    }

    Node::CSptr InnerNode::find(Path const & path) const
    {
        if (path.empty())
        {
            return _tree.root().shared_from_this();
        }

        InnerNode const * current = this;
        size_t index = 0;
        for(; index < path.size() && current; ++index)
        {
            Node::CSptr next = current->find(path[index]);
            current = next ? next->asInnerNode() : nullptr;
            if (!current)
            {
                // not an inner node: finish or fail
                return index == path.size() - 1 ? next : Node::CSptr{};
            }
        }

        return index == path.size() && current ? current->shared_from_this()
                                               : Node::CSptr{};
    }

    void InnerNode::attach(Node & node)
    {
        auto [_, inserted] = _children->emplace(node.id(), node.shared_from_this());
        GENOAF_INVARIANT(inserted, "failed to insert \"{}\" into \"{}\"", node.id(), id());
        markDirty(RevalidationPass::kStructure);
    }

    void InnerNode::detach(Node const & node)
    {
        _children->erase(node.id());
        markDirty(RevalidationPass::kStructure);
    }

    bool InnerNode::updateLevel()
    {
        // TODO: can be implemented w/o recursion
        // TODO: "changed" check can be skipped in Node (it is guaranteed that it is changed)
        //          => just split into updateLevel() and virtual updateLevelImpl()
        if (Node::updateLevel())
        {
            for(auto const & [_, child] : _children.get())
            {
                assert(child);
                child->updateLevel();
            }
            return true;
        }
        return false;
    }
}
