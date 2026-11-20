#include <genoaf/scene/node.hpp>

#include <genoaf/scene/inner-node.hpp>
#include <genoaf/scene/tree.hpp>

#include <algorithm>
#include <cassert>

namespace genoaf::scene
{
    Node::~Node()
    {
        // node must be detached (from parent and, thus, from Tree)
        assert(_parent.get().expired());

        // node must not persist in dirty lists
        assert(std::ranges::all_of(_levelIndeces,
                                   [](size_t i){ return i == kNoIndex; }));
    }

    void Node::markDirty(RevalidationPass const pass)
    {
        if (_level != kNoIndex)
            _tree.markDirty(*this, pass);
    }

    void Node::setParent(std::shared_ptr<InnerNode> const & newParent)
    {
        // early quit (prevents exception on attach())
        auto oldParent = parent();
        if (newParent == oldParent &&
            (newParent || kNoIndex == _level))
        {
            return;
        }

        // copy this's Sptr to extend lifetime to prevent
        // sudden destroy after "oldParent->detach()"
        [[maybe_unused]] Sptr thisGuard = shared_from_this();

        // TODO: if attach or detach throws, the node may become into broken state (double-parented)

        if (newParent)
        {
            newParent->attach(*this);
        }

        _parent = newParent;

        if (oldParent)
        {
            oldParent->detach(*this);
        }

        updateLevel();

        markDirty(RevalidationPass::kStructure);
    }

    void Node::setParent(InnerNode & newParent)
    {
        return setParent(newParent.asInnerNodeSptr());
    }

    bool Node::updateLevel()
    {
        size_t const oldLevel = _level;

        auto parent = _parent.get().lock();
        _level = parent && parent->_level != kNoIndex ? parent->_level + 1
                                                      : kNoIndex;

        bool const changed = oldLevel != _level;
        if (changed)
        {
            _tree.changeLevel(*this, oldLevel, _level);
        }
        return changed;
    }

    Path Node::absPath() const
    {
        Path result;
        for(auto p = shared_from_this(); p; p = p->parent())
        {
            result.emplace_back(p->id());
        }

        assert(!result.empty());
        assert(result.back() == "*");
        result.pop_back();

        std::ranges::reverse(result);
        return result;
    }
}
