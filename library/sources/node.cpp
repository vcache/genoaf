#include <genoaf/node.hpp>

#include <algorithm>

namespace genoaf
{
    // SceneTree

    SceneTree::SceneTree()
        : _root(std::make_shared<InnerNode>(*this, "*"))
    {
        _root->_level = 0; // special case for a root-node
        markDirty(*_root, Pass::kStructure);
    }

    SceneTree::~SceneTree()
    {
        // clean up _dirtyLevel explicitly
        assert(_root);
        assert(0 == _root->_level);
        changeLevel(*_root, _root->_level, kNoIndex);
        _root.reset();
    }

    void SceneTree::finish()
    {
        _minDirtyLevel = _dirtyLevels.size() - 1;
        _maxDirtyLevel = 0;

#       ifndef NDEBUG
        for(DirtyLevel const & dirtyLevel : _dirtyLevels)
        {
            for(size_t pass = 0; pass < kPassSize; ++pass)
            {
                assert(dirtyLevel[pass].empty());
            }
        }
#       endif
    }

    void SceneTree::markDirty(Node & node, SceneTree::Pass pass)
    {
        size_t const passIndex = static_cast<size_t>(pass);
        if (size_t const levelIndex = node._levelIndeces[passIndex];
            levelIndex != kNoIndex)
        {
            assert(node._level < _dirtyLevels.size());
            assert(passIndex < _dirtyLevels[node._level].size());
            assert(levelIndex < _dirtyLevels[node._level][passIndex].size());
            assert(_dirtyLevels[node._level][passIndex][levelIndex] == &node);
            return;
        }

        size_t const level = node.level();
        assert(level != kNoIndex);
        ensureSize(level);

        assert(passIndex < _dirtyLevels[level].size());
        _dirtyLevels[level][passIndex].push_back(&node);

        assert(passIndex < node._levelIndeces.size());
        assert(_dirtyLevels[level][passIndex].size() > 0);
        node._levelIndeces[passIndex] = _dirtyLevels[level][passIndex].size() - 1;

        _minDirtyLevel = std::min(_minDirtyLevel, level);
        _maxDirtyLevel = std::max(_maxDirtyLevel, level);
    }

    void SceneTree::changeLevel(Node & node, size_t oldLevel, size_t newLevel)
    {
        std::array<bool, kPassSize> activePasses;
        activePasses.fill(false);

        // erase from old level
        if (oldLevel != kNoIndex)
        {
            assert(oldLevel < _dirtyLevels.size());
            DirtyLevel & oldDirtyLevel = _dirtyLevels[oldLevel];
            for(size_t passIndex = 0; passIndex < kPassSize; ++passIndex)
            {
                if (size_t const levelIndex = node._levelIndeces[passIndex];
                    levelIndex != kNoIndex)
                {
                    DirtyNodes & dirtyNodes = oldDirtyLevel[passIndex];

                    assert(levelIndex < dirtyNodes.size());
                    assert(dirtyNodes[levelIndex] == &node);
                    dirtyNodes[levelIndex] = dirtyNodes.back();

                    assert(dirtyNodes[levelIndex]);
                    assert(dirtyNodes[levelIndex]->_levelIndeces[passIndex] == dirtyNodes.size() - 1);
                    dirtyNodes[levelIndex]->_levelIndeces[passIndex] = levelIndex;
                    dirtyNodes.pop_back();

                    node._levelIndeces[passIndex] = kNoIndex;
                    activePasses[passIndex] = true;
                }
            }
        }

        // insert into the new one
        if (newLevel != kNoIndex)
        {
            ensureSize(newLevel);
            DirtyLevel & newDirtyLevel = _dirtyLevels[newLevel];
            for(size_t passIndex = 0; passIndex < kPassSize; ++passIndex)
            {
                if (activePasses[passIndex])
                {
                    newDirtyLevel[passIndex].push_back(&node);
                    node._levelIndeces[passIndex] = newDirtyLevel[passIndex].size() - 1;
                }
            }

            // update level boundaries (not shrinking intentionally)
            _minDirtyLevel = std::min(_minDirtyLevel, newLevel);
            _maxDirtyLevel = std::max(_maxDirtyLevel, newLevel);
        }
    }

    void SceneTree::ensureSize(size_t requiredLevel)
    {
        size_t const newSize = std::max(requiredLevel + 1,
                                        _dirtyLevels.size());
        _dirtyLevels.resize(newSize);
    }

    // Node

    Node::~Node()
    {
        // node must be detached (from parent and, thus, from SceneTree)
        assert(_parent.get().expired());

        // node must not persist in dirty lists
        assert(std::ranges::all_of(_levelIndeces, [](size_t i){ return i == kNoIndex; }));
    }

    void Node::setParent(std::shared_ptr<InnerNode> const & newParent)
    {
        // copy this's Sptr to extend lifetime to prevent
        // sudden destroy after "oldParent->detach()"
        [[maybe_unused]] Sptr thisGuard = shared_from_this();

        if (auto oldParent = parent(); oldParent)
        {
            oldParent->detach(*this);
        }

        if (newParent)
        {
            newParent->attach(*this);
        }

        _parent = newParent;
        updateLevel();

        markDirty(SceneTree::Pass::kStructure);
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
            _sceneTree.changeLevel(*this, oldLevel, _level);
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

    // InnerNode

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
            return _sceneTree.root().shared_from_this();
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
        markDirty(SceneTree::Pass::kStructure);
    }

    void InnerNode::detach(Node const & node)
    {
        _children->erase(node.id());
        markDirty(SceneTree::Pass::kStructure);
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
