udcheck 数据协议



一个完整的数据包，包含以下元素

`包头` + `包长` + `数据包` + ... + `数据包` + `和校验`



- 包头

默认以`0xFA`为起始（没什么根据，仅仅是第一次写这个库一直保留下来的习惯），可以自行更改，但是改好后不支持变动，可以多个字节

- 包长

记录整个包的长度，由两个字节表示。大端模式（高字节在前，低字节在后）。数值为`包长`后面字节一直到结尾的字节数

- 数据包

一个数据包由 `id` + `数据长度` + `数据` 组成。

其中，id由一个字节组成。在范围为0-119或120-239，240-255保留由系统使用。（其实最终都被表示为范围在`0-119`的id）

当id在 `0-119` ，表示后面的数据长度较短，及`数据长度位`由一个字节表示，及数据的最大字节数为255。当id在`120-239` 之间时，实际id为减去120。表示后面的数据长度较大，`数据位由两个字节表示`，采用大端模式。

\* 注：id为0表示后面没有数据



- 校验

采用一位和校验，从包头一直加到校验位前