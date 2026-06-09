relevant links:

WSS:
1. Market data feed -
   wss://data.tradingview.com/socket.io/websocket?from=chart%2F3GHJT0rl%2F&date=2026-05-08T11%3A50%3A33&type=chart&auth=sessionid
   The main streaming endpoint. Carries quote/trade/series ticks and chart
   study updates after subscribing with quote_add_symbols / resolve_symbol /
   create_series frames.
     - from=chart/<layoutId>/  : referring page (a saved chart layout); used
                                 server-side for routing/limits and to make
                                 the connection look like it came from the
                                 chart UI.
     - date=<ISO8601 UTC>      : client wall-clock at handshake; acts as a
                                 cache-buster / anti-replay hint.
     - type=chart              : connection profile (chart vs. screener vs.
                                 widget) - affects which symbol scopes the
                                 server lets you subscribe to.
     - auth=sessionid          : value of the `sessionid` cookie from a
                                 logged-in tradingview.com session. Empty
                                 (or the literal "sessionid") = anonymous
                                 tier with delayed data and tighter rate
                                 limits.

2. Private push feed -
   wss://pushstream.tradingview.com/message-pipe-ws/private_feed
   Per-user push channel (NOT market data). Server pushes events tied to
   your account: alert firings, idea/comment notifications, watchlist
   sync, private DMs. Requires a valid `sessionid` cookie on the upgrade
   request - without auth the server closes immediately.

3. Public push feed -
   wss://pushstream.tradingview.com/message-pipe-ws/public
   Broadcast channel any client can connect to anonymously. Used for
   site-wide announcements, public chat rooms, and global event fan-out.
   No auth needed.

HTTP:
   https://data.tradingview.com/socket.io/websocket?from=chart%2F&date=2026-05-08T11%3A50%3A33&type=chart&auth=sessionid
   Same path as (1) but over plain HTTPS. socket.io clients hit this first
   for the handshake / long-polling fallback before negotiating the
   `Upgrade: websocket` switch. We won't use it directly - IXWebSocket
   opens the wss:// upgrade in one shot - but it's handy for poking the
   endpoint with curl to inspect cookies and response headers without
   dealing with WS framing.

WSS (alternatives / siblings of (1)):
4. Premium data feed -
   wss://prodata.tradingview.com/socket.io/websocket
   Real-time, full-depth equivalent of (1). Requires a `sessionid` from a
   Pro / Pro+ / Premium account; the anonymous tier gets 401 on upgrade.

5. Widget data feed -
   wss://widgetdata.tradingview.com/socket.io/websocket
   Lower-privilege endpoint that the embeddable TradingView widgets talk
   to. Accepts anonymous connections more leniently than (1) and is a
   useful fallback when you don't have a `sessionid` yet but still want
   live (delayed) prices.

HTTP (auth / discovery / metadata):
6. Login -
   POST https://www.tradingview.com/accounts/signin/
   Form fields: `username`, `password`, `remember=on`. The 200 response
   body is JSON with `user.session_hash`; the Set-Cookie header is what
   actually matters - the `sessionid` cookie set here is the value you
   feed into the `auth=` query param of (1) and into the `Cookie:` header
   of the WSS upgrade. Needs the same Origin/UA/Referer headers we send
   in bootstrap_tv_session() to avoid the Cloudflare challenge.

7. Current user -
   GET https://www.tradingview.com/accounts/current_user/
   Returns the logged-in user profile JSON, or `{"username":"","..."}` if
   anonymous. Cheap way to verify a `sessionid` is still valid before
   opening the WSS.

8. Symbol search -
   GET
https://symbol-search.tradingview.com/symbol_search/?text=AAPL&type=stock&hl=1&exchange=&lang=en&domain=production
   Resolves a free-text query to canonical `EXCHANGE:TICKER` symbols
   (e.g. `NASDAQ:AAPL`) which is the form (1) requires in
   `quote_add_symbols` / `resolve_symbol` frames.

9. Scanner / screener -
   POST https://scanner.tradingview.com/<market>/scan
   JSON body with `filter`, `columns`, `sort`, `range`. Powers the stock
   screener; the cheapest way to pull a snapshot list (e.g. all S&P 500
   tickers with their last price / volume / change %) without opening a
   WSS at all. `<market>` is one of: america, crypto, forex, etc.

10. History (REST bars) -
    GET
https://history.tradingview.com/v1/history?symbol=NASDAQ:AAPL&resolution=1D&from=<unix>&to=<unix>
    Historical OHLCV bars over plain HTTPS. Equivalent to issuing a
    `create_series` frame on (1) but without holding a socket open -
    better for backfills.

11. News -
    GET
https://news-headlines.tradingview.com/v2/headlines?category=stock&client=web&lang=en&symbol=NASDAQ:AAPL
    GET https://news-headlines.tradingview.com/v2/story?id=<storyId>
    Per-symbol headline list and full-story fetch. No auth required.

12. Economic / earnings calendar -
    GET https://economic-calendar.tradingview.com/events?from=<ISO>&to=<ISO>&countries=US
    GET https://earnings-calendar.tradingview.com/earnings_calendar?from=<ISO>&to=<ISO>
    Macro events and upcoming earnings - useful inputs for scheduling
    around volatility windows.
