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
                <Action ID="IsDoorOpen" />
                <Action ID="PassThroughDoor" />
            </Sequence>

            <Sequence name="door_closed_sequence">
                <Decorator ID="Negation">
                     <Action ID="IsDoorOpen" />
                </Decorator>
                <Decorator ID="RetryUntilSuccesful" num_attempts="4">
                     <Action ID="OpenDoor"/>
                </Decorator>
                <Action ID="PassThroughDoor" />
                <Action ID="CloseDoor" />
            </Sequence>

            <Action ID="PassThroughWindow" />

        </Fallback>
    </BehaviorTree>

    <!-- TreeNodesModel is used only by the Graphic interface -->
    <TreeNodesModel>
        <Action ID="IsDoorOpen" />
        <Action ID="PassThroughDoor" />
        <Action ID="CloseDoor" />
        <Action ID="OpenDoor" />
        <Action ID="PassThroughWindow" />
        <Decorator ID="Negation" />
        <Decorator ID="RetryUntilSuccesful">
            <Parameter label="num_attempts" type="Int" />
        </Decorator>
        <Decorator ID="Repeat">
            <Parameter label="num_cycles" type="Int" />
        </Decorator>
    </TreeNodesModel>
</root>
        )";


#endif // XMLPARSERS_HPP
