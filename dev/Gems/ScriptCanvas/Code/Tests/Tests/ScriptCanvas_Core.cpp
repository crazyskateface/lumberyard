/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#include <Tests/ScriptCanvasTestFixture.h>
#include <Tests/ScriptCanvasTestUtilities.h>
#include <Tests/ScriptCanvasTestNodes.h>

#include <AzCore/Serialization/IdUtils.h>

#include <ScriptCanvas/Core/GraphAsset.h>
#include <ScriptCanvas/Core/GraphAssetHandler.h>

using namespace ScriptCanvasTests;
using namespace TestNodes;

TEST_F(ScriptCanvasTestFixture, AddRemoveSlot)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    AddNodeWithRemoveSlot::Reflect(m_serializeContext);
    AddNodeWithRemoveSlot::Reflect(m_behaviorContext);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("RemoveSlotGraph");
    graphEntity->Init();
    Graph* graph{};
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());

    AZ::EntityId graphUniqueId = graph->GetUniqueId();

    AZ::EntityId outID;
    auto startNode = CreateTestNode<Nodes::Core::Start>(graphUniqueId, outID);
    auto numberAddNode = CreateTestNode<AddNodeWithRemoveSlot>(graphUniqueId, outID);

    auto number1Node = CreateDataNode<Data::NumberType>(graphUniqueId, 1.0, outID);
    auto number2Node = CreateDataNode<Data::NumberType>(graphUniqueId, 2.0, outID);
    auto number3Node = CreateDataNode<Data::NumberType>(graphUniqueId, 4.0, outID);
    auto numberResultNode = CreateDataNode<Data::NumberType>(graphUniqueId, 0.0, outID);

    // data
    EXPECT_TRUE(Connect(*graph, number1Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "A"));
    EXPECT_TRUE(Connect(*graph, number2Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "B"));
    EXPECT_TRUE(Connect(*graph, number3Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "C"));
    EXPECT_TRUE(Connect(*graph, numberAddNode->GetEntityId(), "Result", numberResultNode->GetEntityId(), "Set"));

    // logic
    EXPECT_TRUE(Connect(*graph, startNode->GetEntityId(), "Out", numberAddNode->GetEntityId(), "In"));

    // execute pre remove
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graphEntity->Deactivate();

    if (auto result = numberResultNode->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_DOUBLE_EQ(7.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    // Remove the second number slot from the AddNodeWithRemoveSlot node(the one that contains value = 2.0)
    numberAddNode->RemoveSlot(numberAddNode->GetSlotId("B"));
    EXPECT_FALSE(numberAddNode->GetSlotId("B").IsValid());

    // execute post-remove
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graphEntity->Deactivate();

    if (auto result = numberResultNode->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_DOUBLE_EQ(5.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    // Add two more slots and set there values to 8.0 and 16.0
    EXPECT_TRUE(numberAddNode->AddSlot("D").IsValid());
    EXPECT_TRUE(numberAddNode->AddSlot("B").IsValid());
    auto number4Node = CreateDataNode<Data::NumberType>(graphUniqueId, 8.0, outID);
    auto number5Node = CreateDataNode<Data::NumberType>(graphUniqueId, 16.0, outID);
    EXPECT_TRUE(Connect(*graph, number4Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "D"));
    EXPECT_TRUE(Connect(*graph, number5Node->GetEntityId(), "Get", numberAddNode->GetEntityId(), "B"));

    // execute post-add of slot "D" and re-add of slot "A"
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graphEntity->Deactivate();

    if (auto result = numberResultNode->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_DOUBLE_EQ(1.0 + 4.0 + 8.0 + 16.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    // Remove the first number slot from the AddNodeWithRemoveSlot node(the one that contains value = 1.0
    numberAddNode->RemoveSlot(numberAddNode->GetSlotId("A"));
    EXPECT_FALSE(numberAddNode->GetSlotId("A").IsValid());

    // execute post-remove of "A" slot
    graphEntity->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graphEntity->Deactivate();

    if (auto result = numberResultNode->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_DOUBLE_EQ(4.0 + 8.0 + 16.0, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity.reset();

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    AddNodeWithRemoveSlot::Reflect(m_serializeContext);
    AddNodeWithRemoveSlot::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}


TEST_F(ScriptCanvasTestFixture, NativeProperties)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace Nodes;

    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId addId = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    auto vector3NodeA = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdA);
    auto vector3NodeB = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdB);
    auto vector3NodeC = CreateTestNode<Nodes::Math::Vector3>(graphUniqueId, vector3IdC);

    AZ::EntityId number1Id, number2Id, number3Id, number4Id, number5Id, number6Id, number7Id, number8Id, number9Id;
    Node* number1Node = CreateDataNode<Data::NumberType>(graphUniqueId, 1, number1Id);
    Node* number2Node = CreateDataNode<Data::NumberType>(graphUniqueId, 2, number2Id);
    Node* number3Node = CreateDataNode<Data::NumberType>(graphUniqueId, 3, number3Id);
    Node* number4Node = CreateDataNode<Data::NumberType>(graphUniqueId, 4, number4Id);
    Node* number5Node = CreateDataNode<Data::NumberType>(graphUniqueId, 5, number5Id);
    Node* number6Node = CreateDataNode<Data::NumberType>(graphUniqueId, 6, number6Id);
    Node* number7Node = CreateDataNode<Data::NumberType>(graphUniqueId, 7, number7Id);
    Node* number8Node = CreateDataNode<Data::NumberType>(graphUniqueId, 8, number8Id);
    Node* number9Node = CreateDataNode<Data::NumberType>(graphUniqueId, 0, number9Id);

    // data
    EXPECT_TRUE(Connect(*graph, number1Id, "Get", vector3IdA, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number2Id, "Get", vector3IdA, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number3Id, "Get", vector3IdA, "Number: z"));

    EXPECT_TRUE(Connect(*graph, number4Id, "Get", vector3IdB, "Number: x"));
    EXPECT_TRUE(Connect(*graph, number5Id, "Get", vector3IdB, "Number: y"));
    EXPECT_TRUE(Connect(*graph, number6Id, "Get", vector3IdB, "Number: z"));

    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", addId, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", addId, "Vector3: 1"));

    EXPECT_TRUE(Connect(*graph, addId, "Result: Vector3", vector3IdC, "Set"));

    EXPECT_TRUE(Connect(*graph, vector3IdC, "x: Number", number7Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "y: Number", number8Id, "Set"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "z: Number", number9Id, "Set"));

    // code
    EXPECT_TRUE(Connect(*graph, startID, "Out", addId, "In"));

    graphEntity->Activate();

    EXPECT_FALSE(graph->IsInErrorState());

    if (auto result = vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_EQ(AZ::Vector3(5, 7, 9), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number7Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(5, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number8Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(7, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto result = number9Node->GetInput_UNIT_TEST<Data::NumberType>("Set"))
    {
        EXPECT_EQ(9, *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
}

TEST_F(ScriptCanvasTestFixture, IsNullCheck)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId isNullFalseID;
    CreateTestNode<Nodes::Logic::IsNull>(graphUniqueId, isNullFalseID);

    AZ::EntityId isNullTrueID;
    CreateTestNode<Nodes::Logic::IsNull>(graphUniqueId, isNullTrueID);

    AZ::EntityId isNullResultFalseID;
    auto isNullResultFalse = CreateDataNode(graphUniqueId, true, isNullResultFalseID);
    AZ::EntityId isNullResultTrueID;
    auto isNullResultTrue = CreateDataNode(graphUniqueId, false, isNullResultTrueID);

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");

    AZ::EntityId vectorID;
    Nodes::Core::BehaviorContextObjectNode* vector = CreateTestNode<Nodes::Core::BehaviorContextObjectNode>(graphUniqueId, vectorID);
    const AZStd::string vector3("Vector3");
    vector->InitializeObject(vector3);
    if (auto vector3 = vector->ModInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        *vector3 = AZ::Vector3(1, 1, 1);
    }
    else
    {
        ADD_FAILURE();
    }

    // test the reference type contract
    {
        ScopedOutputSuppression suppressOutput;
        EXPECT_FALSE(Connect(*graph, isNullResultFalseID, "Get", isNullTrueID, "Reference"));
    }
    // data
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", normalizeID, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vectorID, "Get", isNullFalseID, "Reference"));
    EXPECT_TRUE(Connect(*graph, isNullTrueID, "Is Null", isNullResultTrueID, "Set"));
    EXPECT_TRUE(Connect(*graph, isNullFalseID, "Is Null", isNullResultFalseID, "Set"));
    // execution 
    EXPECT_TRUE(Connect(*graph, startID, "Out", isNullTrueID, "In"));
    EXPECT_TRUE(Connect(*graph, isNullTrueID, "True", isNullFalseID, "In"));
    EXPECT_TRUE(Connect(*graph, isNullFalseID, "False", normalizeID, "In"));


    AZ::Entity* graphEntity = graph->GetEntity();

    {
        ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }
    EXPECT_FALSE(graph->IsInErrorState());

    if (auto vector3 = vector->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_TRUE(vector3->IsNormalized());
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto shouldBeFalse = isNullResultFalse->GetInput_UNIT_TEST<bool>("Set"))
    {
        EXPECT_FALSE(*shouldBeFalse);
    }
    else
    {
        ADD_FAILURE();
    }

    if (auto shouldBeTrue = isNullResultTrue->GetInput_UNIT_TEST<bool>("Set"))
    {
        EXPECT_TRUE(*shouldBeTrue);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, NullThisPointerDoesNotCrash)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId normalizeID;
    Nodes::Core::Method* normalize = CreateTestNode<Nodes::Core::Method>(graphUniqueId, normalizeID);
    Namespaces empty;
    normalize->InitializeClass(empty, "Vector3", "Normalize");

    EXPECT_TRUE(Connect(*graph, startID, "Out", normalizeID, "In"));

    AZ::Entity* graphEntity = graph->GetEntity();
    {
        ScopedOutputSuppression suppressOutput;
        graphEntity->Activate();
    }
    // just don't crash, but be in error
    EXPECT_TRUE(graph->IsInErrorState());

    graphEntity->Deactivate();
    delete graphEntity;

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    EventTestHandler::Reflect(m_serializeContext);
    EventTestHandler::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, Assignment)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Data;
    using namespace Nodes;

    AZ::Vector3 min(1, 1, 1);
    AZ::Vector3 max(2, 2, 2);

    AssignTest(AABBType::CreateFromMinMax(min, max), AABBType::CreateFromMinMax(max, min + max));
    AssignTest(AZ::Color(255.f, 127.f, 0.f, 127.f), AZ::Color(0.f, 127.f, 255.f, 127.f));
    AssignTest(CRCType("lhs"), CRCType("rhs"));
    AssignTest(true, false);
    AssignTest(AZ::EntityId(1), AZ::EntityId(2));
    AssignTest(3.0, 4.0);
    AssignTest(Matrix3x3Type::CreateIdentity(), Matrix3x3Type::CreateZero());
    AssignTest(Matrix4x4Type::CreateIdentity(), Matrix4x4Type::CreateZero());
    AssignTest(PlaneType::CreateFromNormalAndDistance(min, 10), PlaneType::CreateFromNormalAndDistance(max, -10));
    AssignTest(OBBType::CreateFromAabb(AABBType::CreateFromMinMax(min, max)), OBBType::CreateFromAabb(AABBType::CreateFromMinMax(max, min + max)));
    AssignTest(StringType("hello"), StringType("good-bye"));
    AssignTest(RotationType::CreateIdentity(), RotationType::CreateZero());
    AssignTest(TransformType::CreateIdentity(), TransformType::CreateZero());
    AssignTest(AZ::Vector2(2, 2), AZ::Vector2(1, 1));
    AssignTest(AZ::Vector3(2, 2, 2), AZ::Vector3(1, 1, 1));
    AssignTest(AZ::Vector4(2, 2, 2, 2), AZ::Vector4(1, 1, 1, 1));

    AZ::Entity* graphEntity = aznew AZ::Entity("Graph");
    graphEntity->Init();
    SystemRequestBus::Broadcast(&SystemRequests::CreateGraphOnEntity, graphEntity);
    auto graph = graphEntity->FindComponent<Graph>();
    EXPECT_NE(nullptr, graph);

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId operatorID;
    CreateTestNode<Nodes::Core::Assign>(graphUniqueId, operatorID);

    AZ::EntityId sourceID;
    Core::BehaviorContextObjectNode* sourceNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, sourceID);
    sourceNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *sourceNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = AZ::Vector3(1, 1, 1);

    AZ::EntityId targetID;
    Core::BehaviorContextObjectNode* targetNode = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, targetID);
    targetNode->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *targetNode->ModInput_UNIT_TEST<AZ::Vector3>("Set") = AZ::Vector3(2, 2, 2);

    EXPECT_TRUE(Connect(*graph, sourceID, "Get", operatorID, "Source"));
    EXPECT_TRUE(Connect(*graph, targetID, "Set", operatorID, "Target"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", operatorID, "In"));

    graphEntity->Activate();

    EXPECT_FALSE(graph->IsInErrorState());

    if (auto result = targetNode->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
    {
        EXPECT_EQ(AZ::Vector3(1, 1, 1), *result);
    }
    else
    {
        ADD_FAILURE();
    }

    graphEntity->Deactivate();
    delete graphEntity;
}


TEST_F(ScriptCanvasTestFixture, ValueTypes)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Datum number0(Datum::CreateInitializedCopy(0));
    int number0Value = *number0.GetAs<int>();

    Datum number1(Datum::CreateInitializedCopy(1));
    int number1Value = *number1.GetAs<int>();

    Datum numberFloat(Datum::CreateInitializedCopy(2.f));
    float numberFloatValue = *numberFloat.GetAs<float>();

    Datum numberDouble(Datum::CreateInitializedCopy(3.0));
    double numberDoubleValue = *numberDouble.GetAs<double>();

    Datum numberHex(Datum::CreateInitializedCopy(0xff));
    int numberHexValue = *numberHex.GetAs<int>();

    Datum numberPi(Datum::CreateInitializedCopy(3.14f));
    float numberPiValue = *numberPi.GetAs<float>();

    Datum numberSigned(Datum::CreateInitializedCopy(-100));
    int numberSignedValue = *numberSigned.GetAs<int>();

    Datum numberUnsigned(Datum::CreateInitializedCopy(100u));
    unsigned int numberUnsignedValue = *numberUnsigned.GetAs<unsigned int>();

    Datum numberDoublePi(Datum::CreateInitializedCopy(6.28));
    double numberDoublePiValue = *numberDoublePi.GetAs<double>();

    EXPECT_EQ(number0Value, 0);
    EXPECT_EQ(number1Value, 1);

    EXPECT_TRUE(AZ::IsClose(numberFloatValue, 2.f, std::numeric_limits<float>::epsilon()));
    EXPECT_TRUE(AZ::IsClose(numberDoubleValue, 3.0, std::numeric_limits<double>::epsilon()));

    EXPECT_NE(number0Value, number1Value);
    EXPECT_FLOAT_EQ(numberPiValue, 3.14f);

    EXPECT_NE(number0Value, numberPiValue);
    EXPECT_NE(numberPiValue, numberDoublePiValue);

    Datum boolTrue(Datum::CreateInitializedCopy(true));
    EXPECT_TRUE(*boolTrue.GetAs<bool>());
    Datum boolFalse(Datum::CreateInitializedCopy(false));
    EXPECT_FALSE(*boolFalse.GetAs<bool>());
    boolFalse = boolTrue;
    EXPECT_TRUE(*boolFalse.GetAs<bool>());
    Datum boolFalse2(Datum::CreateInitializedCopy(false));
    boolTrue = boolFalse2;
    EXPECT_FALSE(*boolTrue.GetAs<bool>());

    {
        const int k_count(10000);
        AZStd::vector<Datum> objects;
        for (int i = 0; i < k_count; ++i)
        {
            objects.push_back(Datum("Vector3", Datum::eOriginality::Original));
        }
    }

    GTEST_SUCCEED();
}


namespace
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    bool GraphHasVectorWithValue(const Graph& graph, const AZ::Vector3& vector)
    {
        for (auto nodeId : graph.GetNodes())
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);

            if (entity)
            {
                if (auto node = AZ::EntityUtils::FindFirstDerivedComponent<Core::BehaviorContextObjectNode>(entity))
                {
                    if (auto candidate = node->GetInput_UNIT_TEST<AZ::Vector3>("Set"))
                    {
                        if (*candidate == vector)
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }
}

TEST_F(ScriptCanvasTestFixture, SerializationSaveTest_Binary)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    // Constant 0
    AZ::Entity* constant0Entity{ aznew AZ::Entity };
    constant0Entity->Init();
    AZ::EntityId constant0Node{ constant0Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, constant0Node, graphUniqueId, Nodes::Math::Number::TYPEINFO_Uuid());

    // Get the node and set the value
    Nodes::Math::Number* constant0NodePtr = nullptr;
    SystemRequestBus::BroadcastResult(constant0NodePtr, &SystemRequests::GetNode<Nodes::Math::Number>, constant0Node);
    EXPECT_TRUE(constant0NodePtr != nullptr);
    if (constant0NodePtr)
    {
        constant0NodePtr->SetInput_UNIT_TEST("Set", 4);
    }

    // Constant 1
    AZ::Entity* constant1Entity{ aznew AZ::Entity };
    constant1Entity->Init();
    AZ::EntityId constant1Node{ constant1Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, constant1Node, graphUniqueId, Nodes::Math::Number::TYPEINFO_Uuid());
    // Get the node and set the value
    Nodes::Math::Number* constant1NodePtr = nullptr;
    SystemRequestBus::BroadcastResult(constant1NodePtr, &SystemRequests::GetNode<Nodes::Math::Number>, constant1Node);
    EXPECT_TRUE(constant1NodePtr != nullptr);
    if (constant1NodePtr)
    {
        constant1NodePtr->SetInput_UNIT_TEST("Set", 12);
    }

    // Sum
    AZ::Entity* sumEntity{ aznew AZ::Entity };
    sumEntity->Init();
    AZ::EntityId sumNode{ sumEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, sumNode, graphUniqueId, Nodes::Math::Sum::TYPEINFO_Uuid());

    AZ::EntityId printID;
    auto printNode = CreateTestNode<TestResult>(graphUniqueId, printID);

    // Connect, disconnect and reconnect to validate disconnection
    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_lhsName), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    graph->Disconnect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_lhsName), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_lhsName), constant0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant0Entity)->GetSlotId("Get"));
    {
        auto* sumNodePtr = AZ::EntityUtils::FindFirstDerivedComponent<Nodes::Math::Sum>(sumEntity);
        EXPECT_NE(nullptr, sumNodePtr);
        EXPECT_EQ(1, graph->GetConnectedEndpoints({ sumNode, sumNodePtr->GetSlotId(BinaryOperator::k_lhsName) }).size());
    }

    graph->Connect(sumNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(sumEntity)->GetSlotId(BinaryOperator::k_rhsName), constant1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(constant1Entity)->GetSlotId("Get"));

    EXPECT_TRUE(Connect(*graph, sumNode, "Result", printID, "Value"));
    EXPECT_TRUE(Connect(*graph, startNode, "Out", sumNode, Nodes::BinaryOperator::k_evaluateName));
    EXPECT_TRUE(Connect(*graph, sumNode, Nodes::BinaryOperator::k_outName, printID, "In"));

    AZ::EntityId vector3IdA, vector3IdB, vector3IdC;
    const AZ::Vector3 allOne(1, 1, 1);
    const AZ::Vector3 allTwo(2, 2, 2);
    const AZ::Vector3 allThree(3, 3, 3);
    const AZ::Vector3 allFour(4, 4, 4);

    Core::BehaviorContextObjectNode* vector3NodeA = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdA);
    vector3NodeA->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeA->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allOne;
    Core::BehaviorContextObjectNode* vector3NodeB = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdB);
    vector3NodeB->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeB->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allTwo;
    Core::BehaviorContextObjectNode* vector3NodeC = CreateTestNode<Core::BehaviorContextObjectNode>(graphUniqueId, vector3IdC);
    vector3NodeC->InitializeObject(azrtti_typeid<AZ::Vector3>());
    *vector3NodeC->ModInput_UNIT_TEST<AZ::Vector3>("Set") = allFour;

    AZ::EntityId add3IdA = CreateClassFunctionNode(graphUniqueId, "Vector3", "Add");

    EXPECT_TRUE(Connect(*graph, printID, "Out", add3IdA, "In"));
    EXPECT_TRUE(Connect(*graph, vector3IdA, "Get", add3IdA, "Vector3: 0"));
    EXPECT_TRUE(Connect(*graph, vector3IdB, "Get", add3IdA, "Vector3: 1"));
    EXPECT_TRUE(Connect(*graph, vector3IdC, "Set", add3IdA, "Result: Vector3"));

    AZStd::unique_ptr<AZ::Entity> graphEntity{ graph->GetEntity() };

    EXPECT_EQ(allOne, *vector3NodeA->GetInput_UNIT_TEST<AZ::Vector3>("Set"));
    EXPECT_EQ(allTwo, *vector3NodeB->GetInput_UNIT_TEST<AZ::Vector3>("Set"));
    EXPECT_EQ(allFour, *vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set"));

    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
    EXPECT_FALSE(GraphHasVectorWithValue(*graph, allThree));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allFour));

    auto nodeIDs = graph->GetNodes();
    EXPECT_EQ(9, nodeIDs.size());

    TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, true);
    graphEntity->Activate();
    TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, false);
    EXPECT_FALSE(graph->IsInErrorState());

    auto vector3C = *vector3NodeC->GetInput_UNIT_TEST<AZ::Vector3>("Set");
    EXPECT_EQ(vector3C, allThree);

    if (auto result = printNode->GetInput_UNIT_TEST<float>("Value"))
    {
        EXPECT_FLOAT_EQ(*result, 16.0f);
    }
    else
    {
        ADD_FAILURE();
    }

    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
    EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
    EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));


    graphEntity->Deactivate();

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    EXPECT_TRUE(fileIO != nullptr);

    // Save it
    bool result = false;
    AZ::IO::FileIOStream outFileStream("serializationTest.scriptCanvas", AZ::IO::OpenMode::ModeWrite);
    if (outFileStream.IsOpen())
    {
        GraphAssetHandler graphAssetHandler;
        AZ::Data::Asset<GraphAsset> graphAsset;
        graphAsset.Create(AZ::Uuid::CreateRandom());
        GraphData saveGraphData;
        graphAsset.Get()->SetGraphData(*graph->GetGraphDataConst());
        EXPECT_TRUE(graphAssetHandler.SaveAssetData(graphAsset, &outFileStream));
        graphAsset.Get()->GetGraphData().Clear(); // Clear GraphData structure to prevent the asset from deleting the graph data
    }

    outFileStream.Close();
}

TEST_F(ScriptCanvasTestFixture, SerializationLoadTest_Binary)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;
    //////////////////////////////////////////////////////////////////////////

    // Make the graph.
    Graph* graph = nullptr;

    AZStd::unique_ptr<AZ::Entity> graphEntity = AZStd::make_unique<AZ::Entity>("Loaded Graph");
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::CreateGraphOnEntity, graphEntity.get());

    auto* fileIO = AZ::IO::LocalFileIO::GetInstance();
    EXPECT_NE(nullptr, fileIO);
    EXPECT_TRUE(fileIO->Exists("serializationTest.ScriptCanvas"));

    // Load it
    AZ::IO::FileIOStream inFileStream("serializationTest.scriptCanvas", AZ::IO::OpenMode::ModeRead);
    if (inFileStream.IsOpen())
    {
        // Serialize the graph entity back in.
        GraphAssetHandler graphAssetHandler;
        AZ::Data::Asset<GraphAsset> graphAsset;
        graphAsset.Create(AZ::Uuid::CreateRandom());
        EXPECT_TRUE(graphAssetHandler.LoadAssetData(graphAsset, &inFileStream, {}));

        GraphData& assetGraphData = graphAsset.Get()->GetGraphData();
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> idRemap;
        AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(&assetGraphData, idRemap, m_serializeContext);
        graph->AddGraphData(assetGraphData);
        // Clear GraphData on asset to prevent it from deleting the graph data, the Graph component takes care of cleaning it up
        assetGraphData.Clear();

        // Close the file
        inFileStream.Close();

        // Initialize the entity
        graphEntity->Init();
        TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, true);
        graphEntity->Activate();
        TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, false);

        // Run the graph component
        graph = graphEntity->FindComponent<Graph>();
        EXPECT_TRUE(graph != nullptr); // "Graph entity did not have the graph component.");
        EXPECT_FALSE(graph->IsInErrorState());

        const AZ::Vector3 allOne(1, 1, 1);
        const AZ::Vector3 allTwo(2, 2, 2);
        const AZ::Vector3 allThree(3, 3, 3);
        const AZ::Vector3 allFour(4, 4, 4);

        auto nodeIDs = graph->GetNodes();
        EXPECT_EQ(9, nodeIDs.size());

        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
        EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));

        // Graph result should be still 16
        // Serialized graph has new remapped entity ids
        auto nodes = graph->GetNodes();
        auto sumNodeIt = AZStd::find_if(nodes.begin(), nodes.end(), [](const AZ::EntityId& nodeId) -> bool
        {
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
            return entity ? AZ::EntityUtils::FindFirstDerivedComponent<TestResult>(entity) ? true : false : false;
        });
        EXPECT_NE(nodes.end(), sumNodeIt);

        TestResult* printNode = nullptr;
        SystemRequestBus::BroadcastResult(printNode, &SystemRequests::GetNode<TestResult>, *sumNodeIt);
        EXPECT_TRUE(printNode != nullptr);

        if (auto result = printNode->GetInput_UNIT_TEST<float>("Value"))
        {
            EXPECT_FLOAT_EQ(*result, 16.0f);
        }
        else
        {
            ADD_FAILURE();
        }

        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allOne));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allTwo));
        EXPECT_TRUE(GraphHasVectorWithValue(*graph, allThree));
        EXPECT_FALSE(GraphHasVectorWithValue(*graph, allFour));
    }
}

TEST_F(ScriptCanvasTestFixture, Contracts)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    ContractNode::Reflect(m_serializeContext);
    ContractNode::Reflect(m_behaviorContext);

    //////////////////////////////////////////////////////////////////////////
    // Make the graph.
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    // Make the nodes.

    // Start
    AZ::Entity* startEntity{ aznew AZ::Entity("Start") };
    startEntity->Init();
    AZ::EntityId startNode{ startEntity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, startNode, graphUniqueId, Nodes::Core::Start::TYPEINFO_Uuid());

    // ContractNode 0
    AZ::Entity* contract0Entity{ aznew AZ::Entity("Contract 0") };
    contract0Entity->Init();
    AZ::EntityId contract0Node{ contract0Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, contract0Node, graphUniqueId, ContractNode::TYPEINFO_Uuid());

    // ContractNode 1
    AZ::Entity* contract1Entity{ aznew AZ::Entity("Contract 1") };
    contract1Entity->Init();
    AZ::EntityId contract1Node{ contract1Entity->GetId() };
    SystemRequestBus::Broadcast(&SystemRequests::CreateNodeOnEntity, contract1Node, graphUniqueId, ContractNode::TYPEINFO_Uuid());

    // invalid
    {
        ScopedOutputSuppression suppressOutput;
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Out")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Get String")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("In"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String")));
        EXPECT_FALSE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set String")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set Number")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get Number")));
        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Set String")));

        EXPECT_FALSE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
    }

    // valid
    EXPECT_TRUE(graph->Connect(startNode, AZ::EntityUtils::FindFirstDerivedComponent<Node>(startEntity)->GetSlotId("Out"), contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set String"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get String")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("In"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Out")));
    EXPECT_TRUE(graph->Connect(contract0Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract0Entity)->GetSlotId("Set Number"), contract1Node, AZ::EntityUtils::FindFirstDerivedComponent<Node>(contract1Entity)->GetSlotId("Get Number")));

    // Execute it
    graph->GetEntity()->Activate();
    EXPECT_FALSE(graph->IsInErrorState());
    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

#if defined (SCRIPTCANVAS_ERRORS_ENABLED)

TEST_F(ScriptCanvasTestFixture, Error)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    ScriptUnitTestEventHandler::Reflect(m_serializeContext);
    ScriptUnitTestEventHandler::Reflect(m_behaviorContext);
    UnitTestErrorNode::Reflect(m_serializeContext);
    UnitTestErrorNode::Reflect(m_behaviorContext);
    InfiniteLoopNode::Reflect(m_serializeContext);
    InfiniteLoopNode::Reflect(m_behaviorContext);

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Unit test error!";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<UnitTestErrorNode>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorNodeID, "Out", sideEffectID, "In"));

    // test to make sure the graph received an error
    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }
    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(description, graph->GetLastErrorDescription());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 0);

    graph->GetEntity()->Deactivate();
    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorHandled)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Unit test error!";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZStd::string sideFX2 = "Side FX 2";
    AZ::EntityId sideFX2NodeID;
    CreateDataNode(graphUniqueId, sideFX2, sideFX2NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<UnitTestErrorNode>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID1 = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");
    AZ::EntityId sideEffectID2 = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);

    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID1, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideFX2NodeID, "Get", sideEffectID2, "String: 0"));
    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorNodeID, "Out", sideEffectID1, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Source", errorNodeID, "This"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectID2, "In"));

    {
        ScopedOutputSuppression supressOutput; // temporarily suppress the output, since we don't fully support error handling correction, yet
        graph->GetEntity()->Activate();
    }
    
    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 0);
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX2), 1);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorNode)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    // Start
    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZStd::string description = "Test Error Report";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId errorNodeID;
    CreateTestNode<Nodes::Core::Error>(graphUniqueId, errorNodeID);

    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, startID, "Out", sideEffectID, "In"));
    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideEffectID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, stringNodeID, "Get", errorNodeID, "Description"));

    // test to make sure the graph received an error
    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }
    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(description, graph->GetLastErrorDescription());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    graph->GetEntity()->Deactivate();

    // test to make sure the graph did to retry execution
    graph->GetEntity()->Activate();
    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    // if the graph was allowed to execute, the side effect counter should be higher
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, ErrorNodeHandled)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZStd::string description = "Test Error Report";
    AZ::EntityId stringNodeID;
    CreateDataNode(graphUniqueId, description, stringNodeID);

    AZStd::string sideFX1 = "Side FX 1";
    AZ::EntityId sideFX1NodeID;
    CreateDataNode(graphUniqueId, sideFX1, sideFX1NodeID);

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId errorNodeID;
    CreateTestNode<Nodes::Core::Error>(graphUniqueId, errorNodeID);
    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);
    AZ::EntityId sideEffectID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, startID, "Out", errorNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Source", errorNodeID, "This"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectID, "In"));
    EXPECT_TRUE(Connect(*graph, sideFX1NodeID, "Get", sideEffectID, "String: 0"));

    {
        ScopedOutputSuppression supressOutput; // temporarily suppress the output, since we don't fully support error handling correction, yet
        graph->GetEntity()->Activate();
    }

    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 1);

    graph->GetEntity()->Deactivate();
    graph->GetEntity()->Activate();

    EXPECT_FALSE(graph->IsInErrorState());
    EXPECT_FALSE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(unitTestHandler.SideEffectCount(sideFX1), 2);

    delete graph->GetEntity();
}

TEST_F(ScriptCanvasTestFixture, InfiniteLoopDetected)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    UnitTestEventsHandler unitTestHandler;
    unitTestHandler.BusConnect();

    AZStd::string sideFXpass = "Side FX Infinite Loop Error Handled";
    AZ::EntityId sideFXpassNodeID;
    CreateDataNode(graphUniqueId, sideFXpass, sideFXpassNodeID);

    AZStd::string sideFXfail = "Side FX Infinite Loop Node Failed";
    AZ::EntityId sideFXfailNodeID;
    CreateDataNode(graphUniqueId, sideFXfail, sideFXfailNodeID);

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);
    AZ::EntityId infiniteLoopNodeID;
    CreateTestNode<InfiniteLoopNode>(graphUniqueId, infiniteLoopNodeID);
    AZ::EntityId errorHandlerID;
    CreateTestNode<Nodes::Core::ErrorHandler>(graphUniqueId, errorHandlerID);

    AZ::EntityId sideEffectPassID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");
    AZ::EntityId sideEffectFailID = CreateClassFunctionNode(graphUniqueId, "UnitTestEventsBus", "SideEffect");

    EXPECT_TRUE(Connect(*graph, sideFXpassNodeID, "Get", sideEffectPassID, "String: 0"));
    EXPECT_TRUE(Connect(*graph, sideFXfailNodeID, "Get", sideEffectFailID, "String: 0"));

    EXPECT_TRUE(Connect(*graph, startID, "Out", infiniteLoopNodeID, "In"));
    EXPECT_TRUE(Connect(*graph, infiniteLoopNodeID, "In", infiniteLoopNodeID, "Before Infinity"));

    EXPECT_TRUE(Connect(*graph, infiniteLoopNodeID, "After Infinity", sideEffectFailID, "In"));
    EXPECT_TRUE(Connect(*graph, errorHandlerID, "Out", sideEffectPassID, "In"));

    {
        ScopedOutputSuppression suppressOutput;
        graph->GetEntity()->Activate();
    }

    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXpass));
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXfail));

    graph->GetEntity()->Deactivate();
    graph->GetEntity()->Activate();

    EXPECT_TRUE(graph->IsInErrorState());
    EXPECT_TRUE(graph->IsInIrrecoverableErrorState());
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXpass));
    EXPECT_EQ(0, unitTestHandler.SideEffectCount(sideFXfail));

    delete graph->GetEntity();
}

#endif // SCRIPTCANAS_ERRORS_ENABLED


TEST_F(ScriptCanvasTestFixture, BinaryOperationTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::Nodes::Comparison;

    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);

    BinaryOpTest<EqualTo>(1, 1, true);
    BinaryOpTest<EqualTo>(1, 0, false);
    BinaryOpTest<EqualTo>(0, 1, false);
    BinaryOpTest<EqualTo>(0, 0, true);

    BinaryOpTest<NotEqualTo>(1, 1, false);
    BinaryOpTest<NotEqualTo>(1, 0, true);
    BinaryOpTest<NotEqualTo>(0, 1, true);
    BinaryOpTest<NotEqualTo>(0, 0, false);

    BinaryOpTest<Greater>(1, 1, false);
    BinaryOpTest<Greater>(1, 0, true);
    BinaryOpTest<Greater>(0, 1, false);
    BinaryOpTest<Greater>(0, 0, false);

    BinaryOpTest<GreaterEqual>(1, 1, true);
    BinaryOpTest<GreaterEqual>(1, 0, true);
    BinaryOpTest<GreaterEqual>(0, 1, false);
    BinaryOpTest<GreaterEqual>(0, 0, true);

    BinaryOpTest<Less>(1, 1, false);
    BinaryOpTest<Less>(1, 0, false);
    BinaryOpTest<Less>(0, 1, true);
    BinaryOpTest<Less>(0, 0, false);

    BinaryOpTest<LessEqual>(1, 1, true);
    BinaryOpTest<LessEqual>(1, 0, false);
    BinaryOpTest<LessEqual>(0, 1, true);
    BinaryOpTest<LessEqual>(0, 0, true);

    BinaryOpTest<EqualTo>(true, true, true);
    BinaryOpTest<EqualTo>(true, false, false);
    BinaryOpTest<EqualTo>(false, true, false);
    BinaryOpTest<EqualTo>(false, false, true);

    BinaryOpTest<NotEqualTo>(true, true, false);
    BinaryOpTest<NotEqualTo>(true, false, true);
    BinaryOpTest<NotEqualTo>(false, true, true);
    BinaryOpTest<NotEqualTo>(false, false, false);

    AZStd::string string0("a string");
    AZStd::string string1("b string");

    BinaryOpTest<EqualTo>(string1, string1, true);
    BinaryOpTest<EqualTo>(string1, string0, false);
    BinaryOpTest<EqualTo>(string0, string1, false);
    BinaryOpTest<EqualTo>(string0, string0, true);

    BinaryOpTest<NotEqualTo>(string1, string1, false);
    BinaryOpTest<NotEqualTo>(string1, string0, true);
    BinaryOpTest<NotEqualTo>(string0, string1, true);
    BinaryOpTest<NotEqualTo>(string0, string0, false);

    BinaryOpTest<Greater>(string1, string1, false);
    BinaryOpTest<Greater>(string1, string0, true);
    BinaryOpTest<Greater>(string0, string1, false);
    BinaryOpTest<Greater>(string0, string0, false);

    BinaryOpTest<GreaterEqual>(string1, string1, true);
    BinaryOpTest<GreaterEqual>(string1, string0, true);
    BinaryOpTest<GreaterEqual>(string0, string1, false);
    BinaryOpTest<GreaterEqual>(string0, string0, true);

    BinaryOpTest<Less>(string1, string1, false);
    BinaryOpTest<Less>(string1, string0, false);
    BinaryOpTest<Less>(string0, string1, true);
    BinaryOpTest<Less>(string0, string0, false);

    BinaryOpTest<LessEqual>(string1, string1, true);
    BinaryOpTest<LessEqual>(string1, string0, false);
    BinaryOpTest<LessEqual>(string0, string1, true);
    BinaryOpTest<LessEqual>(string0, string0, true);

    const AZ::Vector3 vectorOne(1.0f, 1.0f, 1.0f);
    const AZ::Vector3 vectorZero(0.0f, 0.0f, 0.0f);

    BinaryOpTest<EqualTo>(vectorOne, vectorOne, true);
    BinaryOpTest<EqualTo>(vectorOne, vectorZero, false);
    BinaryOpTest<EqualTo>(vectorZero, vectorOne, false);
    BinaryOpTest<EqualTo>(vectorZero, vectorZero, true);

    BinaryOpTest<NotEqualTo>(vectorOne, vectorOne, false);
    BinaryOpTest<NotEqualTo>(vectorOne, vectorZero, true);
    BinaryOpTest<NotEqualTo>(vectorZero, vectorOne, true);
    BinaryOpTest<NotEqualTo>(vectorZero, vectorZero, false);

    const TestBehaviorContextObject zero(0);
    const TestBehaviorContextObject one(1);
    const TestBehaviorContextObject otherOne(1);

    BinaryOpTest<EqualTo>(one, one, true);
    BinaryOpTest<EqualTo>(one, zero, false);
    BinaryOpTest<EqualTo>(zero, one, false);
    BinaryOpTest<EqualTo>(zero, zero, true);

    BinaryOpTest<NotEqualTo>(one, one, false);
    BinaryOpTest<NotEqualTo>(one, zero, true);
    BinaryOpTest<NotEqualTo>(zero, one, true);
    BinaryOpTest<NotEqualTo>(zero, zero, false);

    BinaryOpTest<Greater>(one, one, false);
    BinaryOpTest<Greater>(one, zero, true);
    BinaryOpTest<Greater>(zero, one, false);
    BinaryOpTest<Greater>(zero, zero, false);

    BinaryOpTest<GreaterEqual>(one, one, true);
    BinaryOpTest<GreaterEqual>(one, zero, true);
    BinaryOpTest<GreaterEqual>(zero, one, false);
    BinaryOpTest<GreaterEqual>(zero, zero, true);

    BinaryOpTest<Less>(one, one, false);
    BinaryOpTest<Less>(one, zero, false);
    BinaryOpTest<Less>(zero, one, true);
    BinaryOpTest<Less>(zero, zero, false);

    BinaryOpTest<LessEqual>(one, one, true);
    BinaryOpTest<LessEqual>(one, zero, false);
    BinaryOpTest<LessEqual>(zero, one, true);
    BinaryOpTest<LessEqual>(zero, zero, true);

    BinaryOpTest<EqualTo>(one, otherOne, true);
    BinaryOpTest<EqualTo>(otherOne, one, true);

    BinaryOpTest<NotEqualTo>(one, otherOne, false);
    BinaryOpTest<NotEqualTo>(otherOne, one, false);

    BinaryOpTest<Greater>(one, otherOne, false);
    BinaryOpTest<Greater>(otherOne, one, false);

    BinaryOpTest<GreaterEqual>(one, otherOne, true);
    BinaryOpTest<GreaterEqual>(otherOne, one, true);

    BinaryOpTest<Less>(one, otherOne, false);
    BinaryOpTest<Less>(otherOne, one, false);

    BinaryOpTest<LessEqual>(one, otherOne, true);
    BinaryOpTest<LessEqual>(otherOne, one, true);

    // force failures, to verify crash proofiness
    BinaryOpTest<EqualTo>(one, zero, false, true);
    BinaryOpTest<NotEqualTo>(one, zero, true, true);
    BinaryOpTest<Greater>(one, zero, true, true);
    BinaryOpTest<GreaterEqual>(one, zero, true, true);
    BinaryOpTest<Less>(one, zero, false, true);
    BinaryOpTest<LessEqual>(one, zero, false, true);

    m_serializeContext->EnableRemoveReflection();
    m_behaviorContext->EnableRemoveReflection();
    TestBehaviorContextObject::Reflect(m_serializeContext);
    TestBehaviorContextObject::Reflect(m_behaviorContext);
    m_serializeContext->DisableRemoveReflection();
    m_behaviorContext->DisableRemoveReflection();
}

TEST_F(ScriptCanvasTestFixture, EntityRefTest)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);

    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;

    // Fake "world" entities
    AZ::Entity first("First");
    first.CreateComponent<ScriptCanvasTests::TestComponent>();
    first.Init();
    first.Activate();

    AZ::Entity second("Second");
    second.CreateComponent<ScriptCanvasTests::TestComponent>();
    second.Init();
    second.Activate();

    // Script Canvas Graph
    Graph* graph = nullptr;
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);

    graph->GetEntity()->SetName("Graph Owner Entity");

    graph->GetEntity()->CreateComponent<ScriptCanvasTests::TestComponent>();
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityId = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    // EntityRef node to some specific entity #1
    AZ::EntityId selfEntityIdNode;
    auto selfEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, selfEntityIdNode);
    selfEntityRef->SetEntityRef(first.GetId());

    // EntityRef node to some specific entity #2
    AZ::EntityId otherEntityIdNode;
    auto otherEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, otherEntityIdNode);
    otherEntityRef->SetEntityRef(second.GetId());

    // Explicitly set an EntityRef node with this graph's entity Id.
    AZ::EntityId graphEntityIdNode;
    auto graphEntityRef = CreateTestNode<Nodes::Entity::EntityRef>(graphUniqueId, graphEntityIdNode);
    graphEntityRef->SetEntityRef(graph->GetEntity()->GetId());

    // First test: directly set an entity Id on the EntityID: 0 slot
    AZ::EntityId eventAid;
    Nodes::Core::Method* nodeA = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventAid);
    nodeA->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = nodeA->ModInput_UNIT_TEST<AZ::EntityId>("EntityID: 0"))
    {
        *entityId = first.GetId();
    }

    // Second test: Will connect the slot to an EntityRef node
    AZ::EntityId eventBid;
    Nodes::Core::Method* nodeB = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventBid);
    nodeB->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");

    // Third test: Set the slot's EntityId: 0 to SelfReferenceId, this should result in the same Id as graph->GetEntityId()
    AZ::EntityId eventCid;
    Nodes::Core::Method* nodeC = CreateTestNode<Nodes::Core::Method>(graphUniqueId, eventCid);
    nodeC->InitializeEvent({ {} }, "EntityRefTestEventBus", "TestEvent");
    if (auto entityId = nodeC->ModInput_UNIT_TEST<AZ::EntityId>("EntityID: 0"))
    {
        *entityId = ScriptCanvas::SelfReferenceId;
    }

    // True 
    AZ::EntityId trueNodeId;
    CreateDataNode<Data::BooleanType>(graphUniqueId, true, trueNodeId);

    // False
    AZ::EntityId falseNodeId;
    CreateDataNode<Data::BooleanType>(graphUniqueId, false, falseNodeId);

    // Start            -> TestEvent
    //                   O EntityId: 0    (not connected, it was set directly on the slot)
    // EntityRef: first -O EntityId: 1
    // False            -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventAid, "In"));
    EXPECT_TRUE(Connect(*graph, eventAid, "EntityID: 1", selfEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventAid, "Boolean: 2", falseNodeId, "Get"));

    // Start            -> TestEvent
    // EntityRef: second -O EntityId: 0 
    // EntityRef: second -O EntityId: 1
    // False             -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventBid, "In"));
    EXPECT_TRUE(Connect(*graph, eventBid, "EntityID: 0", otherEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventBid, "EntityID: 1", otherEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventBid, "Boolean: 2", falseNodeId, "Get"));

    // Start            -> TestEvent
    //                   -O EntityId: 0  (not connected, slot is set to SelfReferenceId)
    // graphEntityIdNode -O EntityId: 1
    // True              -O Boolean: 2
    EXPECT_TRUE(Connect(*graph, startID, "Out", eventCid, "In"));
    EXPECT_TRUE(Connect(*graph, eventCid, "EntityID: 1", graphEntityIdNode, "Get"));
    EXPECT_TRUE(Connect(*graph, eventCid, "Boolean: 2", trueNodeId, "Get"));

    // Execute the graph
    graph->ResolveSelfReferences(graphEntityId);

    TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, true);
    graph->GetEntity()->Activate();
    TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, false);
    EXPECT_FALSE(graph->IsInErrorState());
    delete graph->GetEntity();

}

const int k_executionCount = 998;

TEST_F(ScriptCanvasTestFixture, ExecutionLength)
{
    RETURN_IF_TEST_BODIES_ARE_DISABLED(TEST_BODY_DEFAULT);
    using namespace ScriptCanvas;

    Graph* graph(nullptr);
    SystemRequestBus::BroadcastResult(graph, &SystemRequests::MakeGraph);
    EXPECT_TRUE(graph != nullptr);
    graph->GetEntity()->Init();

    const AZ::EntityId& graphEntityID = graph->GetEntityId();
    const AZ::EntityId& graphUniqueId = graph->GetUniqueId();

    AZ::EntityId startID;
    CreateTestNode<Nodes::Core::Start>(graphUniqueId, startID);

    AZ::EntityId previousID = startID;

    for (int i = 0; i < k_executionCount; ++i)
    {
        AZ::EntityId printNodeID;
        TestResult* printNode = CreateTestNode<TestResult>(graphUniqueId, printNodeID);
        printNode->SetInput_UNIT_TEST<int>("Value", i);
        EXPECT_TRUE(Connect(*graph, previousID, "Out", printNodeID, "In"));
        previousID = printNodeID;
    }

    AZ::Entity* graphEntity = graph->GetEntity();
    TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, true);
    graphEntity->Activate();
    TraceSuppressionBus::Broadcast(&TraceSuppression::SuppressPrintf, false);
    EXPECT_FALSE(graph->IsInErrorState());

    graphEntity->Deactivate();
    delete graphEntity;
}