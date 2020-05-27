//
// Created by inFinity on 2019/12/5.
//

#ifndef KALEIDO_COMMON_H
#define KALEIDO_COMMON_H

template<typename T, typename F>
std::ostream &operator<<(std::ostream &os, const std::pair<T, F> &p) {
    os << "(" << p.first << ", " << p.second << ")";
    return os;
}

#endif //KALEIDO_COMMON_H
