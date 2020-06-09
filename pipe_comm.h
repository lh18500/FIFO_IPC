//
// Created by lh18500 on 3/11/20.
//

#ifndef FIFOSERVER_PIPE_COMM_H
#define FIFOSERVER_PIPE_COMM_H

#include "FIFO.h"
#include "util/blockingconcurrentqueue.h"
#include <stdio.h>
#include <string.h>
#include <thread>
#define REQUEST_PIPE "REQUEST_PIPE"
#define RESPONSE_PIPE "RESPONSE_PIPE"
#define SERVER_MODE 0
#define CLIENT_MODE 1
#define DEQUEUE_SIZE 100
#define MAX_DATA_SIZE 65536

namespace communication
{
    class pipe_comm {
    public:
        struct data_bundle{
            char * data;
            int64_t size;
        };

    private:
        std::atomic<bool> enabled;
        FIFO read_end;
        FIFO write_end;
        moodycamel::BlockingConcurrentQueue<data_bundle*> read_queue;
        moodycamel::BlockingConcurrentQueue<data_bundle*> write_queue;

    public:
        pipe_comm(int64_t mode, const std::string & request_pipe, const std::string & response_pipe) : enabled(true) {
            switch (mode) {
                case SERVER_MODE:
                    read_end.make_fifo(request_pipe, O_RDONLY);
                    write_end.make_fifo(response_pipe, O_WRONLY);
                    break;
                case CLIENT_MODE:
                    read_end.make_fifo(response_pipe, O_RDONLY);
                    write_end.make_fifo(request_pipe, O_WRONLY);
                    break;
                default:
                    exit(EXIT_FAILURE);
            }
            auto t1 = std::thread(&pipe_comm::read_task, this);
            auto t2 = std::thread(&pipe_comm::write_task, this);
            t1.detach();
            t2.detach();
        }

        ~pipe_comm() {
            enabled = false;
        }

        bool add_to_write(data_bundle * bundle) {
            if (write_end.fifo_can_write()) {
                return write_queue.enqueue(bundle);
            }
            else return false;
        }

        int64_t get_bulk_from_read(data_bundle ** bundle, int64_t count) {
            return read_queue.wait_dequeue_bulk(bundle, static_cast<unsigned long>(count));
        }

        void write_task() {
            data_bundle* msg_arr[DEQUEUE_SIZE];
            while (enabled) {
                int64_t count = write_queue.wait_dequeue_bulk(msg_arr, DEQUEUE_SIZE);
                for (int ii = 0; ii < count; ii++) {
                    auto write_bytes = write_end.write_to_fifo(msg_arr[ii]->data, static_cast<size_t>(msg_arr[ii]->size));
                    if (write_bytes <= 0) {
                        printf("Failed to write %li.\n", msg_arr[ii]->size);
                    }
                }
            }
        }

        void read_task() {
            //const char s[2] = "\0";
            char buf[MAX_DATA_SIZE];
            while (enabled) {
                int64_t size = read_end.read_from_fifo(buf, MAX_DATA_SIZE, 500);
                if (size > 0) {
                    //char * token = strtok(buf, s);
                    char * curr = buf;
                    while (curr && curr < (buf + size)) {
                        auto * bundle = (data_bundle*)malloc(sizeof(data_bundle));
                        bundle->data = (char*)malloc(strlen(curr) + 1);
                        strcpy(bundle->data, curr);
                        bundle->size = strlen(bundle->data);
                        while (!read_queue.enqueue(bundle)) {
                            printf("Failed to enqueue bundle.");
                        }
                        curr += (strlen(curr) + 1);
                    }
                }
            }
        }
    };
}

#endif //FIFOSERVER_PIPE_COMM_H
