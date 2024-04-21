-module(memcache_txt).
-export([start/1,send_msg/2,close/1]).

%start :: SockAddr -> pid()
% Genera la conexión del servidor en la dirección SockAddr
% en modo binario. Devuelve el identificador del proceso que
% maneja los pedidos al server.
start(Host) ->
    {ok, S} = gen_tcp:connect(Host, 888, [binary, {active, false}]),
    PidServer = spawn (fun() -> server(S) end),
    PidServer.

    

server(S) ->
    receive
        {From, close} ->
            From ! {self(), gen_tcp:close(S)};
        {From, Msg} -> 
           case gen_tcp:send(S, Msg) of
            ok ->
                From ! {self(), recv_msg(S)};
            Error ->
                From ! {self(), Error}
            end,
            server(S)
    end.

recv_msg(S) ->
    case gen_tcp:recv(S, 0) of
        {ok, Msg} -> Msg;
        Error -> Error
    end.


send_msg(ServerPID , Msg) ->
    ServerPID ! {self(), Msg},
    receive
        {ServerPID, Rsp} -> Rsp;
        Error -> Error
    end.

close(ServerPID) ->
    ServerPID ! {self(), close},
    receive
        {ServerPID, ok} -> ok;
        {ServerPID, Error} -> Error
    end.
