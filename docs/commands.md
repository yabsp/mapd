# Commands and Information Related to Memory Management

## Check for Fragmentation

To check for fragmentation from the Linux kernel's perspective, you can use:

```bash
sudo cat /proc/buddyinfo
```

The output will look like this:

| Node | Zone   |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  | 10 |
|------|--------|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|----|
| 0    | DMA    |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |  3 |
| 0    | DMA32  |  3  |  3  |  2  |  3  |  3  |  4  |  3  |  4  |  3  |  5  |857 |
| 0    | Normal |1111 | 823 |231  | 67  | 36  | 18  |  9  | 12  | 14  | 13  |672 |


### Explanation of the Output

- **Node N**: Represents NUMA nodes  
  `Node 0` means only one node — no multi-socket system.
- **Zones**:
  - **DMA**: Legacy zone for low-memory devices (below 1 MB or 16 MB)
  - **DMA32**: 32-bit addressable memory (first 4 GB of RAM)
  - **Normal**: Regular memory used for kernel and userspace
- **Numbers (Orders 0–10)**: Represent how many free memory blocks are available of each size.

### Order Table

| Order | Pages | Block Size |
|:-----:|:-----:|:-----------:|
|   0   |   1   | 4 KB        |
|   1   |   2   | 8 KB        |
|   2   |   4   | 16 KB       |
|  ...  |  ...  | ...         |
|  10   | 1024  | 4 MB        |

## Interpretation

If there are a lot of tiny blocks and very few large ones, the system may **not be able to allocate large pages**, even if enough total RAM is available — this is a sign of **fragmentation**.

> **Example**: Only lowest two order blocks have large counts (e.g. >2000 in order 0), and all higher orders are 0 → bad fragmentation.

## Monitoring Buddy Info Live

Use `watch` to refresh the output every few seconds:

```bash
watch -n t cat /proc/buddyinfo
```

Replace `t` with the time interval in seconds.

> `watch -n t <command>` will repeat the command every `t` seconds.
