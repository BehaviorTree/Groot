#ifndef XMLPARSERS_HPP
#define XMLPARSERS_HPP

#include "tinyXML2/tinyxml2.h"
#include "bt_editor_base.h"

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>



void ReadTreeNodesModel(const tinyxml2::XMLElement* root,
                        QtNodes::DataModelRegistry& registry,
                        TreeNodeModels& models_list);

void RecursivelyCreateXml(const QtNodes::FlowScene &scene,
                          tinyxml2::XMLDocument& doc,
                          tinyxml2::XMLElement* parent_element,
                          const QtNodes::Node* node);



//--------------------------------------------------------

const QString gTestXML = R"(

<root main_tree_to_execute = "MainTree" >

    <BehaviorTree ID="MainTree">
        <Fallback name="root_Fallback">

            <Sequence name="door_open_sequence">
                <Condition ID="IsDoorOpen" />
                <Action ID="PassThroughDoor" />
            </Sequence>

            <SubTree ID="DoorClosed"/>

            <Action ID="PassThroughWindow" />

        </Fallback>
    </BehaviorTree>

    <BehaviorTree ID="DoorClosed">
        <Sequence name="door_closed_sequence">
            <Decorator ID="Negation">
                <Condition ID="IsDoorOpen" />
            </Decorator>
            <Decorator ID="RetryUntilSuccesful" num_attempts="4">
                <Action ID="OpenDoor"/>
            </Decorator>
            <Action ID="PassThroughDoor" />
            <Action ID="CloseDoor" />
       </Sequence>
    </BehaviorTree>

    <!-- TreeNodesModel is used only by the Graphic interface -->
    <TreeNodesModel>
        <Condition ID="IsDoorOpen" />
        <Action ID="PassThroughDoor" />
        <Action ID="CloseDoor" />
        <Action ID="OpenDoor" />
        <Action ID="PassThroughWindow" />
    </TreeNodesModel>
</root>
        )";


#endif // XMLPARSERS_HPP
