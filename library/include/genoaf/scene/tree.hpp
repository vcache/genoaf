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
        // The callback's signature is the following:
        //      void (Node &)
        //
        // It is assumed that the callback will revalidate node,
        // therefore, it can be removed from dirty-list.
        //
        // Erased or detached nodes won't be revalidated, but
        // there are ways to handle this case:
        //  - in Node's dtor
        //  - in parent's node (_children will be changed)
        template<typename Function>
        void revalidate(RevalidationPass pass, Function function)
        {
            if (_minDirtyLevel > _maxDirtyLevel ||
                _minDirtyLevel == kNoIndex)
            {
                return;
            }

            size_t const passIndex = static_cast<size_t>(pass);
            for(size_t level = _minDirtyLevel; level <= _maxDirtyLevel; ++level)
            {
                assert(level < _dirtyLevels.size());
                DirtyLevel & dirtyLevel = _dirtyLevels[level];

                assert(passIndex < dirtyLevel.size());
                DirtyNodes & dirtyNodes = dirtyLevel[passIndex];

                while(!dirtyNodes.empty())
                {
                    assert(dirtyNodes.back());
                    Node & current = *dirtyNodes.back();

                    function(current);

                    assert(current._levelIndeces[passIndex] == dirtyNodes.size() - 1);
                    current._levelIndeces[passIndex] = kNoIndex;

                    dirtyNodes.pop_back();
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
        size_t                     _minDirtyLevel = kNoIndex;
        size_t                     _maxDirtyLevel = 0;

        friend class Node;
    };
}
