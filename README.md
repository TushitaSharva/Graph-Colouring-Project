# Graph Colouring Project

## Conventions for Our Algorithm:

- 0th process will be the master process/master node.
- The graph has 1-based indexes.
- Initially, all nodes will have colour as '0', which means they are uncoloured.
- They should be coloured from 1, 2, ... N colours where N is the number of vertices.
- There is an array maintained by each process called colour, which represents its knowledge of the colours of the graph at that moment.
- Overall, the types of messages are:
  - COLOUR: Process node to process node (every time) as well as to master node (when it changes colour) --> will be discussed properly in the algo.
  - CHECK: Master node to process nodes saying that it had come to know every process has decided its colour at least once, so check if the current colour array the process has is consistent.
  - ACK: Process node to Master node saying all its colours are consistent according to its adjacency list.
  - FINISH: Master node to process node saying that it has received an ACK from a node, and also it attaches the consistent coloring with the message. On receiving this, every process will decide its colour according to the message received, and they will exit.

## Algorithm:

### PROCESS NODES:

1. When the coloring process is initiated, the 1st process will be assigned the colour 1 (i.e., colour[1] will be 1 now), and it will send the updated colour array to all of its neighbors. From now on, every process will be executing the same set of rules.

2. When a node receives a message, it might be in a colored state or it might be in an uncoloured state.
   - It will first identify the colours of all the neighbours. It will maintain two sets, available and unavailable. Available is initialized to all acceptable colours {1..N} and unavailable to an empty set.
   - If its colour and another neighbor's colour are the same:
     - It will check the pid of the neighbor.
       - Only if the neighbor's pid is high:
         - It will remove the color of the neighbor from available and add it to unavailable. (This is to avoid a cycle of each changing to the same colour)
       - Else it will let that be, and when this message is received by the neighbor's pid, it will be its responsibility to change the colour.

3. Given it now has the list of unavailable colours, it will change only if its colour is amongst the unavailable. Even if it can take the least available at this point, it might unnecessarily involve changing of many other connected nodes.

4. Now, from the received message, it will update all the colors that are not the neighbors. This should not affect because these are not direct neighbors.

5. If the MASTER process has previously sent the CHECK message, then at this point, it will check if the graph it has is consistent.
   - If consistent it will send the ACK to the MASTER process saying it has a coloring that is consistent.

6. At the end, it will send all its neighbors the color array it has with it, and come back to the loop.
   - The loop terminates on receiving FINISH from MASTER.

### MASTER NODE:

1. At first, it will keep listening for process nodes' COLOUR messages. If all 1 ... N processes sent the COLOUR message, it will send a CHECK message to the processes.

2. Now it will wait to receive that one ACK message from process, and upon receiving, it will disperse it and return.
