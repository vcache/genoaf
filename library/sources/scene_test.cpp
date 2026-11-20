#include <gtest/gtest.h>

#include <genoaf/scene/inner-node.hpp>
#include <genoaf/scene/node.hpp>
#include <genoaf/scene/tree.hpp>

#include <unordered_map>

namespace genoaf::scene::test
{
    // Basic structural operations

    TEST(Tree, SmokeTest)
    {
        Tree tree;

        EXPECT_TRUE(tree.root().empty());
        EXPECT_EQ(tree.root().size(), 0);

        EXPECT_EQ(tree.root().absPath(), Path{});

        EXPECT_EQ(tree.root().begin(),
                  tree.root().end());

        EXPECT_FALSE(tree.root().contains(""));
        EXPECT_FALSE(tree.root().contains("foo"));
        EXPECT_FALSE(tree.root().contains(Path{"*"}));

        EXPECT_TRUE(tree.root().contains(Path()));
        ASSERT_TRUE(tree.root().find(Path()));
        EXPECT_EQ(tree.root().find(Path()).get(),
                  &tree.root());

        EXPECT_EQ(tree.root().id(), "*");
        EXPECT_EQ(tree.root().level(), 0);
    }

    TEST(Tree, LeafInsert)
    {
        Tree tree;
        auto & root = tree.root();

        auto & node = root.emplace<Node>("leaf-1");
        EXPECT_EQ(node.absPath(), Path{"leaf-1"});
        EXPECT_EQ(node.id(), "leaf-1");
        EXPECT_EQ(node.level(), 1);

        EXPECT_FALSE(root.empty());
        EXPECT_EQ(root.size(), 1);

        EXPECT_TRUE(root.contains("leaf-1"));
        EXPECT_FALSE(root.contains("leaf-2"));

        EXPECT_TRUE(root.contains(Path{"leaf-1"}));
        EXPECT_FALSE(root.contains(Path{"leaf-1", "foo"}));
        EXPECT_FALSE(root.contains(Path{"foo", "leaf-1"}));
        EXPECT_FALSE(root.contains(Path{"leaf-2"}));
    }

    TEST(Tree, TreeAddressing)
    {
        Tree tree;
        auto & root = tree.root();

        auto & level1 = root.emplace<InnerNode>("level-1");
        auto & level2 = level1.emplace<InnerNode>("level-2");
        auto & level3 = level2.emplace<Node>("level-3");

        EXPECT_EQ(level1.level(), 1);
        EXPECT_EQ(level2.level(), 2);
        EXPECT_EQ(level3.level(), 3);

        EXPECT_EQ(level1.id(), "level-1");
        EXPECT_EQ(level2.id(), "level-2");
        EXPECT_EQ(level3.id(), "level-3");

        EXPECT_TRUE(root.contains(Path{"level-1"}));
        EXPECT_TRUE(root.contains(Path{"level-1", "level-2"}));
        EXPECT_TRUE(root.contains(Path{"level-1", "level-2", "level-3"}));

        EXPECT_EQ(level1.absPath(), (Path{"level-1"}));
        EXPECT_EQ(level2.absPath(), (Path{"level-1", "level-2"}));
        EXPECT_EQ(level3.absPath(), (Path{"level-1", "level-2", "level-3"}));

        EXPECT_EQ(root.find(Path{"level-1"}), level1.shared_from_this());
        EXPECT_EQ(root.find<InnerNode>(Path{"level-1", "level-2"}), level2.shared_from_this());
        EXPECT_EQ(root.find(Path{"level-1", "level-2", "level-3"}), level3.shared_from_this());
    }

    TEST(Tree, At)
    {
        Tree tree;
        auto & root = tree.root();

        root.emplace<InnerNode>("inner");
        EXPECT_NO_THROW(root.at<InnerNode>("inner"));
        EXPECT_NO_THROW(root.at<Node>("inner")); // InnerNode is-a Node
        EXPECT_THROW(root.at<InnerNode>("foo"), genoaf::Exception);

        EXPECT_NO_THROW(root.at<InnerNode>(Path{"inner"}));
        EXPECT_THROW(root.at<InnerNode>(Path{"foo", "inner"}),
                     genoaf::Exception);
        EXPECT_THROW(root.at<InnerNode>(Path{"inner", "foo"}),
                     genoaf::Exception);

        root.at<InnerNode>("inner").emplace<Node>("leaf");

        EXPECT_NO_THROW(root.at<Node>(Path{"inner", "leaf"}));
        EXPECT_THROW(root.at<InnerNode>(Path{"inner", "leaf"}),
                     genoaf::Exception);

    }

    TEST(Tree, RemoveTrivial)
    {
        Tree tree;
        auto & root = tree.root();

        root.emplace<Node>("foo");
        ASSERT_TRUE(root.contains("foo"));
        ASSERT_EQ(root.size(), 1);

        root.erase("foo");
        EXPECT_FALSE(root.contains("foo"));
        EXPECT_EQ(root.size(), 0);
    }

    TEST(Tree, RemoveSubtreeByErase)
    {
        Tree tree;
        auto & root = tree.root();

        Node::Wptr level3 = root.emplace<InnerNode>("level-1")
                                .emplace<InnerNode>("level-2")
                                .emplace<InnerNode>("level-3")
                                .weak_from_this();

        ASSERT_FALSE(level3.expired());

        root.at<InnerNode>("level-1").erase("level-2");

        ASSERT_TRUE(root.contains("level-1"));
        EXPECT_TRUE(root.at<InnerNode>("level-1").empty());

        EXPECT_TRUE(level3.expired());
    }

    TEST(Tree, RemoveSubtreeByParent)
    {
        Tree tree;
        auto & root = tree.root();

        Node::Wptr level3 = root.emplace<InnerNode>("level-1")
                                .emplace<InnerNode>("level-2")
                                .emplace<InnerNode>("level-3")
                                .weak_from_this();

        ASSERT_FALSE(level3.expired());

        root.at<InnerNode>("level-1")
            .at<InnerNode>("level-2")
            .setParent({});

        ASSERT_TRUE(root.contains("level-1"));
        EXPECT_TRUE(root.at<InnerNode>("level-1").empty());

        EXPECT_TRUE(level3.expired());
    }

    TEST(Tree, Clear)
    {
        Tree tree;
        auto & root = tree.root();

        root.emplace<InnerNode>("level-1")
            .emplace<InnerNode>("level-2")
            .emplace<Node>("level-3");

        root.clear();
        EXPECT_TRUE(root.empty());
    }

    TEST(Tree, Reparent)
    {
        Tree tree;
        auto & root = tree.root();

        root.emplace<InnerNode>("level-1a")
            .emplace<InnerNode>("level-2a")
            .emplace<InnerNode>("level-3a");

        InnerNode & newParent = root.emplace<InnerNode>("level-1b");

        auto target = root.find(Path{"level-1a", "level-2a", "level-3a"});
        ASSERT_TRUE(target);
        ASSERT_EQ(target->level(), 3);

        target->setParent(newParent);

        EXPECT_EQ(target->level(), 2);

        EXPECT_FALSE(root.contains(Path{"level-1a", "level-2a", "level-3a"}));
        EXPECT_TRUE(root.contains(Path{"level-1a", "level-2a"}));
        EXPECT_TRUE(root.contains(Path{"level-1b", "level-3a"}));
    }

    // Dirty lists and revalidation

    namespace
    {
        // NOTE: Path here is not "a path in the tree", but more
        //       like a revalidation log
        std::unordered_map<RevalidationPass, Path>
        revalidateAndLog(Tree & tree)
        {
            std::unordered_map<RevalidationPass, Path> result;
            for(size_t passIndex = 0; passIndex < kRevalidationPassCount; ++passIndex)
            {
                RevalidationPass const pass = static_cast<RevalidationPass>(passIndex);
                Path & subResult = result[pass];
                tree.revalidate(pass,
                    [&subResult](Node const & node) -> bool
                    {
                        subResult.emplace_back(node.id());
                        return true;
                    });
            }
            tree.finish();
            return result;
        }
    }

    TEST(Tree, StructuralPass)
    {
        // build the testbed
        Tree tree;
        auto & root = tree.root();
        root.emplace<InnerNode>("foo");

        // perform revalidation pass
        auto firstPass = revalidateAndLog(tree);
        EXPECT_EQ(firstPass.at(RevalidationPass::kStructure), (Path{"*", "foo"}));

        // ensure it won't be revalidated twice
        auto secondPass = revalidateAndLog(tree);
        EXPECT_TRUE(secondPass.at(RevalidationPass::kStructure).empty());
    }

    TEST(Tree, MarkDirty)
    {
        // build the testbed
        Tree tree;
        auto & root = tree.root();
        root.emplace<InnerNode>("foo")
            .emplace<Node>("bar");

        // mark dirty
        root.at<Node>(Path{"foo", "bar"})
            .markDirty(RevalidationPass::kTransforms);

        // check which passes has been revalidated
        auto logs = revalidateAndLog(tree);
        EXPECT_EQ(logs.at(RevalidationPass::kStructure), (Path{"*", "foo", "bar"}));
        EXPECT_TRUE(logs.at(RevalidationPass::kProperties).empty());
        EXPECT_TRUE(logs.at(RevalidationPass::kAnimation).empty());
        EXPECT_TRUE(logs.at(RevalidationPass::kOrigins).empty());
        EXPECT_EQ(logs.at(RevalidationPass::kTransforms), (Path{"bar"}));
        EXPECT_TRUE(logs.at(RevalidationPass::kOutlines).empty());
        EXPECT_TRUE(logs.at(RevalidationPass::kVisibility).empty());
    }

    TEST(Tree, ReparentDirtySubtree)
    {
        // build the testbed
        Tree tree;
        auto & root = tree.root();
        root.emplace<InnerNode>("left");
        root.emplace<InnerNode>("right");

        root.at<InnerNode>("left").emplace<InnerNode>("sub-tree-a")
                                  .emplace<InnerNode>("sub-tree-b")
                                  .emplace<InnerNode>("sub-tree-c");

        // mark dirty
        root.at<Node>(Path{"left", "sub-tree-a", "sub-tree-b"})
            .markDirty(RevalidationPass::kTransforms);
        root.at<Node>(Path{"left", "sub-tree-a", "sub-tree-b", "sub-tree-c"})
            .markDirty(RevalidationPass::kTransforms);

        // reparent the subtree
        root.at<Node>(Path{"left", "sub-tree-a"}).setParent(
            root.find<InnerNode>("right"));

        // check pass revalidation log
        auto logs = revalidateAndLog(tree);
        EXPECT_EQ(logs.at(RevalidationPass::kTransforms),
                  (Path{"sub-tree-b", "sub-tree-c"}));
    }
}