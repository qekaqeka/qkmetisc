### Why does this project exist?

This project was supposed to be a more optimized version of socat or something else. This project doesn't create proceses or threads for each connection. All connections are handled by workers, which number you can specify by using -w or --workers-nr command line options.



The architecture is designed for creating CTF tasks based on this project. You can find several examples of tasks in "gens" folder.



How the systems works:

- Plots

- Plot tasks

- Gens

- Tasks

- Questions



Plots provide plot tasks which are based on gens' tasks. The only difference between them is that plot tasks have timeout.

Gens generate tasks (for example eq_gen), which give questions. Questions have text, that send over the network to a client. The clients answer is checked in task_check function. This check can return 3 different results:

- ANSWER_WRONG (which disconnects client)

- ANSWER_MORE (need more information from client)

- ANSWER_RIGHT (which gives the next task)



### Why this project shouldn't exist

The problem that this project solves doesn't really exist. Nobody wants a framework to create hypersonic speed handlers for network CTF tasks. Also you need to write your CTF task code in C, not in python (which is much more convenient). You need to do this inside the source tree. I think it's obvious why it's a terrible design. Even Linux kernel supports modules, but this project doesnt' :-).

If you read source code of this project, you will notice that this project is literally a reinvention of the wheel. The only thing which I didn't reinvent is GMP, which is used in eq_gen code. The reason for it is that this project is educational. And I have "educated" enough




