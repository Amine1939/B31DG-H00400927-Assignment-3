# B31DG-H00400927-Assignment-3
Project Structure:\
main/\
├── main.c              # FreeRTOS task creation, SYNC detection, ISR setup\
├── tasks.c/.h          # Task logic, WorkKernel calls, token publication\
├── gpio.c/.h           # GPIO configuration and ACK pin helpers\
├── edge_counter.c/.h   # PCNT initialisation and delta-per-period reads\
├── monitor.c/.h        # Software timing monitor (deadline tracking)\
├── workkernel.c        # Provided WorkKernel function (do not modify)\
└── CMakeLists.txt
