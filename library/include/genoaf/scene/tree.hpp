#pragma once

#include <genoaf/misc.hpp>
#include <genoaf/scene/node.hpp>
#include <genoaf/scene/revalidation-pass.hpp>

#include <array>
#include <cassert>
#include <memory>
#include <vector>

namespace genoaf::scene
{
    class InnerNode;

    // Tree, a class holding root and track changes
    class Tree
    {
        GENOAF_NO_COPY(Tree);
        GENOAF_NO_MOVE(Tree);

    public:
        Tree();
        ~Tree();

        InnerNode & root() { assert(_root); return *_root; }
        InnerNode const & root() const { assert(_root); return *_root; }

    public:
        // Function signature is the following:
        //      bool(Node &)
        //
        // It must return true if the node has been
        // revalidated, and it can be removed from dirty-list.
        template<typename Function>
        void revalidate(RevalidationPass pass, Function function)
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

        // Must be called after all revalidation passes
        void finish();

    private:
        void markDirty(Node & node, RevalidationPass pass);
        void changeLevel(Node & node, size_t oldLevel, size_t newLevel);
        void ensureSize(size_t requiredLevel);

    private:
        // The structure:
        //  Level 0
        //   ...
        //  Level i
        //      DirtyNodes (RevalidationPass::kAnimation)
        //      DirtyNodes (RevalidationPass::kOrigins)
        //       ...
        //   ...
        //  Level N-1

        using DirtyNodes = std::vector<Node *>;
        using DirtyLevel = std::array<DirtyNodes, kRevalidationPassCount>;

        std::shared_ptr<InnerNode> _root;
        std::vector<DirtyLevel>    _dirtyLevels;
        size_t                     _minDirtyLevel = 0;
        size_t                     _maxDirtyLevel = 0;

        friend class Node;
    };
}