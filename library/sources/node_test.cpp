#include <gtest/gtest.h>

#include <genoaf/node.hpp>

#include <unordered_map>

namespace genoaf::test
{
    // Basic structural operations

    TEST(SceneTree, SmokeTest)
    {
        SceneTree sceneTree;

        EXPECT_TRUE(sceneTree.root().empty());
        EXPECT_EQ(sceneTree.root().size(), 0);

        EXPECT_EQ(sceneTree.root().absPath(), Path{});

        EXPECT_EQ(sceneTree.root().begin(),
                  sceneTree.root().end());

        EXPECT_FALSE(sceneTree.root().contains(""));
        EXPECT_FALSE(sceneTree.root().contains("foo"));
        EXPECT_FALSE(sceneTree.root().contains(Path{"*"}));

        EXPECT_TRUE(sceneTree.root().contains(Path()));
        ASSERT_TRUE(sceneTree.root().find(Path()));
        EXPECT_EQ(sceneTree.root().find(Path()).get(),
                  &sceneTree.root());

        EXPECT_EQ(sceneTree.root().id(), "*");
        EXPECT_EQ(sceneTree.root().level(), 0);
    }

    TEST(SceneTree, LeafInsert)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

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

    TEST(SceneTree, TreeAddressing)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

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

    TEST(SceneTree, At)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

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

    TEST(SceneTree, RemoveTrivial)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

        root.emplace<Node>("foo");
        ASSERT_TRUE(root.contains("foo"));
        ASSERT_EQ(root.size(), 1);

        root.erase("foo");
        EXPECT_FALSE(root.contains("foo"));
        EXPECT_EQ(root.size(), 0);
    }

    TEST(SceneTree, RemoveSubtreeByErase)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

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

    TEST(SceneTree, RemoveSubtreeByParent)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

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

    TEST(SceneTree, Clear)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

        root.emplace<InnerNode>("level-1")
            .emplace<InnerNode>("level-2")
            .emplace<Node>("level-3");

        root.clear();
        EXPECT_TRUE(root.empty());
    }

    TEST(SceneTree, Reparent)
    {
        SceneTree sceneTree;
        auto & root = sceneTree.root();

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
        std::unordered_map<SceneTree::Pass, Path>
        revalidateAndLog(SceneTree & sceneTree)
        {
            std::unordered_map<SceneTree::Pass, Path> result;
            for(size_t passIndex = 0; passIndex < SceneTree::kPassSize; ++passIndex)
            {
                SceneTree::Pass const pass = static_cast<SceneTree::Pass>(passIndex);
                Path & subResult = result[pass];
                sceneTree.revalidate(pass,
                    [&subResult](Node const & node) -> bool
                    {
                        subResult.emplace_back(node.id());
                        return true;
                    });
            }
            sceneTree.finish();
            return result;
        }
    }

    TEST(SceneTree, StructuralPass)
    {
        // build the testbed
        SceneTree sceneTree;
        auto & root = sceneTree.root();
        root.emplace<InnerNode>("foo");

        // perform revalidation pass
        auto firstPass = revalidateAndLog(sceneTree);
        EXPECT_EQ(firstPass.at(SceneTree::Pass::kStructure), (Path{"*", "foo"}));

        // ensure it won't be revalidated twice
        auto secondPass = revalidateAndLog(sceneTree);
        EXPECT_TRUE(secondPass.at(SceneTree::Pass::kStructure).empty());
    }

    TEST(SceneTree, MarkDirty)
    {
        // build the testbed
        SceneTree sceneTree;
        auto & root = sceneTree.root();
        root.emplace<InnerNode>("foo")
            .emplace<Node>("bar");

        // mark dirty
        root.at<Node>(Path{"foo", "bar"})
            .markDirty(SceneTree::Pass::kTransforms);

        // check which passes has been revalidated
        auto logs = revalidateAndLog(sceneTree);
        EXPECT_EQ(logs.at(SceneTree::Pass::kStructure), (Path{"*", "foo", "bar"}));
        EXPECT_TRUE(logs.at(SceneTree::Pass::kProperties).empty());
        EXPECT_TRUE(logs.at(SceneTree::Pass::kAnimation).empty());
        EXPECT_TRUE(logs.at(SceneTree::Pass::kOrigins).empty());
        EXPECT_EQ(logs.at(SceneTree::Pass::kTransforms), (Path{"bar"}));
        EXPECT_TRUE(logs.at(SceneTree::Pass::kOutlines).empty());
        EXPECT_TRUE(logs.at(SceneTree::Pass::kVisibility).empty());
    }

    TEST(SceneTree, ReparentDirtySubtree)
    {
        // build the testbed
        SceneTree sceneTree;
        auto & root = sceneTree.root();
        root.emplace<InnerNode>("left");
        root.emplace<InnerNode>("right");

        root.at<InnerNode>("left").emplace<InnerNode>("sub-tree-a")
                                  .emplace<InnerNode>("sub-tree-b")
                                  .emplace<InnerNode>("sub-tree-c");

        // mark dirty
        root.at<Node>(Path{"left", "sub-tree-a", "sub-tree-b"})
            .markDirty(SceneTree::Pass::kTransforms);
        root.at<Node>(Path{"left", "sub-tree-a", "sub-tree-b", "sub-tree-c"})
            .markDirty(SceneTree::Pass::kTransforms);

        // reparent the subtree
        root.at<Node>(Path{"left", "sub-tree-a"}).setParent(
            root.find<InnerNode>("right"));

        // check pass revalidation log
        auto logs = revalidateAndLog(sceneTree);
        EXPECT_EQ(logs.at(SceneTree::Pass::kTransforms),
                  (Path{"sub-tree-b", "sub-tree-c"}));
    }
}