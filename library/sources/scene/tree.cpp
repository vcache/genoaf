#include <genoaf/scene/tree.hpp>

#include <genoaf/scene/inner-node.hpp>

#include <algorithm>

namespace genoaf::scene
{
    Tree::Tree()
        : _root(std::make_shared<InnerNode>(*this, "*"))
    {
        _root->_level = 0; // special case for a root-node
        markDirty(*_root, RevalidationPass::kStructure);
    }

    Tree::~Tree()
    {
        // clean up _dirtyLevel explicitly
        assert(_root);
        assert(0 == _root->_level);
        changeLevel(*_root, _root->_level, kNoIndex);
        _root.reset();
    }

    void Tree::finish()
    {
        _minDirtyLevel = _dirtyLevels.size() - 1;
        _maxDirtyLevel = 0;

#       ifndef NDEBUG
        for(DirtyLevel const & dirtyLevel : _dirtyLevels)
        {
            for(size_t pass = 0; pass < kRevalidationPassCount; ++pass)
            {
                assert(dirtyLevel[pass].empty());
            }
        }
#       endif
    }

    void Tree::markDirty(Node & node, RevalidationPass pass)
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

    void Tree::changeLevel(Node & node, size_t oldLevel, size_t newLevel)
    {
        std::array<bool, kRevalidationPassCount> activePasses;
        activePasses.fill(false);

        // erase from old level
        if (oldLevel != kNoIndex)
        {
            assert(oldLevel < _dirtyLevels.size());
            DirtyLevel & oldDirtyLevel = _dirtyLevels[oldLevel];
            for(size_t passIndex = 0; passIndex < kRevalidationPassCount; ++passIndex)
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
            for(size_t passIndex = 0; passIndex < kRevalidationPassCount; ++passIndex)
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

    void Tree::ensureSize(size_t requiredLevel)
    {
        size_t const newSize = std::max(requiredLevel + 1,
                                        _dirtyLevels.size());
        _dirtyLevels.resize(newSize);
    }
}
