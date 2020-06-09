//
// Created by lh18500 on 3/9/20.
//

#ifndef FIFOSERVER_FIFO_H
#define FIFOSERVER_FIFO_H

#include "string"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

namespace communication {
    class FIFO {
    private:
        std::string name_;
        int type_;

    public:
        FIFO() : type_(-1){};
        FIFO(const std::string & name, int type) {
            mknod(name.c_str(), S_IFIFO|0666, 0);
            name_ = name;
            type_ = type;
        }
        ~FIFO() = default;

        void make_fifo(const std::string & name, int type) {
            mknod(name.c_str(), S_IFIFO|0666, 0);
            name_ = name;
            type_ = type;
        }

        bool fifo_can_write() {
            if (type_ != O_WRONLY && type_ != O_RDWR) {
                return false;
            }
            int fd = open(name_.c_str(), O_WRONLY | O_NONBLOCK);
            if (fd != -1) {
                close(fd);
                return true;
            }
            else return false;
        }

        ssize_t read_from_fifo(char* buf, size_t size, int to_msec) {
            if (type_ != O_RDONLY && type_ != O_RDWR) {
                return -1;
            }
            int64_t to_microsec = to_msec * 1000;
            struct timeval start{}, stop{};
            gettimeofday(&start, nullptr);
            gettimeofday(&stop, nullptr);
            //clock_t t_begin;
            //t_begin = clock();
            size_t read_bytes = 0;
            int fd = open(name_.c_str(), O_RDWR | O_NONBLOCK);
            fd_set read_set;
            timeval tm{};
            while (read_bytes < size && (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec < to_microsec) {
                FD_ZERO(&read_set);
                tm.tv_sec = 0;
                tm.tv_usec = 500000;
                FD_SET(fd, &read_set);
                ssize_t n = select(fd + 1, &read_set, nullptr, nullptr, &tm);
                if (n > 0) {
                    n = read(fd, buf + read_bytes, size - read_bytes);
                    if (n <= 0) {
                        int temp_fd = open(name_.c_str(), O_RDWR | O_NONBLOCK);
                        if (temp_fd == -1) {
                            close(fd);
                            return errno;
                        }
                        close(fd);
                        return read_bytes;
                    }
                    else {
                        read_bytes += n;
                        gettimeofday(&stop, nullptr);
                    }
                }
                else {
                    close(fd);
                    return read_bytes;
                }
            }
            close(fd);
            return read_bytes;
        }

        ssize_t write_to_fifo(const char* buf, size_t size) {
            if (type_ != O_WRONLY && type_ != O_RDWR) {
                return -1;
            }
            if (!buf || !size) {
                return -1;
            }
            int fd = open(name_.c_str(), O_WRONLY | O_NONBLOCK);
            if (fd < 0) {
                return -1;
            }
            fd_set send_set;
            timeval tm{};
            size_t sent_bytes = 0;
            while (sent_bytes < size) {
                FD_ZERO(&send_set);
                tm.tv_sec = 1;
                tm.tv_usec = 0;
                FD_SET(fd, &send_set);

                ssize_t n = select(fd + 1, nullptr, &send_set, nullptr, &tm);
                if (n > 0) {
                    n = write(fd, buf + sent_bytes, size - sent_bytes);
                    if (n <= 0) {
                        int temp_fd = open(name_.c_str(), O_WRONLY | O_NONBLOCK);
                        if (temp_fd == -1) {
                            close(fd);
                            return errno;
                        }
                        close(fd);
                        return sent_bytes;
                    }
                    else {
                        sent_bytes += n;
                    }
                }
                else {
                    close(fd);
                    return -1;
                }
            }
            close(fd);
            return sent_bytes;
        }
    };
}

#endif //FIFOSERVER_FIFO_H
