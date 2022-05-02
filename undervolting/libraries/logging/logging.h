#pragma once

namespace logging {

void open(char const *ip, int port);
void close();

void write(char const *format, ...);

} // namespace logging
