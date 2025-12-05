# Logic-Maze
Logic Maze Database System - Complete Project Documentation
Project Overview
Name: Logic Grid - Database-Backed Logic Puzzle Game
Concept: A 10×10 grid logic puzzle where each cell has a hidden TRUE/FALSE value. Players
reveal cells to discover clues about other cells and use logical deduction to determine all values.
The entire system is backed by a custom-built persistent database engine.
Architecture: 3-tier application
Tier 1: Storage Engine (C++ - disk-based persistence)
Tier 2: Backend API Server (C++ - business logic)
Tier 3: Frontend GUI (HTML/CSS/JavaScript - runs on laptop)
Core Requirements Met
✅ Persistent B-Tree Storage Engine - All data stored on disk, pages loaded on-demand
✅ Multiple Data Structures - B+ Trees (primary indexes), Hash Tables (exact lookups),
B-Trees (graph traversal)
✅ Hybrid Caching Strategy - LRU buffer pool manager
✅ Page-based Storage - 8KB pages, slotted page format
✅ Complex B-Tree Operations - Insert with split, delete with merge, range queries
✅ Real-world Application - Fully functional logic puzzle game
✅ 3-tier Architecture - Separated storage, logic, and presentation layers
Game Mechanics
Grid Structure:
10×10 grid = 100 cells
Each cell has a hidden truth value (TRUE/FALSE)
Gameplay:
Player clicks unrevealed cell → reveals truth value + clue
Clue describes other cells (e.g., "Exactly 3 cells in Row 5 are TRUE")
Player uses logic to deduce other cells without revealing them
Goal: Correctly deduce all 100 cells
Clue Types:
Row/Column Count: "Exactly N cells in Row X are TRUE"
Corner: "Only 1 corner cell is FALSE"

Adjacent: "All adjacent cells are TRUE"
Region: "Top-left quadrant has 12 TRUE cells"
Pattern: "Diagonal cells alternate TRUE/FALSE"
Example Flow:
Open (0,0) → Shows FALSE, Clue: "Only 1 corner is FALSE"
Logic: Since (0,0) is FALSE and only 1 corner is FALSE
→ Other corners (0,9), (9,0), (9,9) must be TRUE
Player marks those cells as TRUE (deduction)
Database Schema
Table 1: GAME_SESSIONS
Tracks all game instances.
Columns:
session_id (INT, PRIMARY KEY)
player_name (VARCHAR)
difficulty (ENUM: easy/medium/hard/expert)
grid_size (INT, default 10)
revealed_cells (INT)
correctly_deduced (INT)
start_time (TIMESTAMP)
end_time (TIMESTAMP)
status (ENUM: active/won/abandoned)
Indexes:
B+ Tree on session_id (primary)
Hash Table on player_name
B+ Tree on (status, start_time) for leaderboard
Table 2: GRID_CELLS
Stores all cells in all games.
Columns:
cell_id (INT, PRIMARY KEY)
session_id (INT, FOREIGN KEY)
row_index (INT, 0-9)
col_index (INT, 0-9)

actual_truth_value (BOOLEAN) - hidden answer
is_revealed (BOOLEAN)
player_deduction (BOOLEAN, nullable)
clue_text (TEXT)
clue_type (VARCHAR)
Indexes:
B+ Tree on cell_id (primary)
Hash Table on (session_id, row_index, col_index) - O(1) position lookup
B+ Tree on (session_id, is_revealed) - get unrevealed cells
B+ Tree on (session_id, row_index) - row queries for clue validation
B+ Tree on (session_id, col_index) - column queries
Table 3: CLUE_REFERENCES
Defines which cells/regions each clue constrains.
Columns:
clue_id (INT, PRIMARY KEY)
source_cell_id (INT, FK) - cell containing the clue
clue_type (VARCHAR: row/column/corner/adjacent/region)
constraint_operator (VARCHAR: exactly/at_least/at_most)
constraint_value (INT)
target_region_json (TEXT) - JSON describing which cells
Indexes:
B+ Tree on clue_id (primary)
B-Tree on source_cell_id - graph traversal for validation
Example JSON:
Row clue: {"type":"row","row":5}
Corner clue: {"type":"corners","cells":[[0,0],[0,9],[9,0],[9,9]]}
Adjacent clue: {"type":"adjacent","center":[5,5]}
Table 4: PLAYER_MOVES
Records every action (for undo, analytics).
Columns:

move_id (INT, PRIMARY KEY)
session_id (INT, FK)
move_number (INT)
move_type (ENUM: reveal/deduce/verify)
cell_id (INT, FK)
guess_value (BOOLEAN)
timestamp (TIMESTAMP)
Indexes:
B+ Tree on move_id (primary)
B+ Tree on (session_id, move_number) - ordered move history
Table 5: LEADERBOARD
High scores.
Columns:
entry_id (INT, PRIMARY KEY)
player_name (VARCHAR)
difficulty (VARCHAR)
completion_time (INT, seconds)
total_moves (INT)
accuracy_percent (FLOAT)
date_achieved (TIMESTAMP)
Indexes:
B+ Tree on entry_id (primary)
B+ Tree on (difficulty, completion_time) - top scores query
Storage Engine Architecture
Layer 1: Disk Manager
Responsibilities:
Manage single database file (database.db)
Allocate/deallocate pages (8KB blocks)
Read/write pages from/to disk
Maintain free page list
Key Operations:

readPage(page_id) → load 8KB from disk
writePage(page_id, data) → write 8KB to disk
allocatePage() → returns new page_id
deallocatePage(page_id) → mark as free
File Structure:
Page 0: Database header (metadata, root pointers)
Page 1-N: Data/index pages
Each page: 8192 bytes (128 bytes header + 8064 bytes data)
Layer 2: Buffer Pool Manager
Responsibilities:
In-memory cache of hot pages
LRU eviction policy
Pin/unpin mechanism (prevent eviction of active pages)
Dirty page tracking and flush
Configuration:
Pool size: 100 pages (800 KB RAM)
Typical game uses ~20 pages
Expected hit rate: 95%+
Key Operations:
fetchPage(page_id) → Page* (loads from disk if not cached)
unpinPage(page_id, is_dirty) → allow eviction
flushPage(page_id) → write dirty page to disk
flushAllPages() → write all dirty pages
LRU Replacer:
Maintains doubly-linked list of unpinned pages
Hash map for O(1) access
victim() returns least recently used unpinned page
Layer 3: Index Structures
B+ Tree (Primary Indexes)
Properties:

Order: 128 (fan-out)
All data in leaf nodes
Leaf nodes linked for range scans
Internal nodes: keys + child pointers only
Use Cases:
Primary key indexes (cell_id, session_id)
Range queries: "Get cells in rows 3-7"
Sequential scans: "Get all sessions ordered by time"
Operations:
search(key) → O(log n) → returns record_id
rangeSearch(start_key, end_key) → O(log n + k) → returns list of record_ids
insert(key, value) → O(log n), may trigger splits
delete(key) → O(log n), may trigger merges
Split Logic:
Leaf node splits when full (>128 keys)
Promote middle key to parent
Recursively split parent if needed
May increase tree height
Hash Table (Exact Match Lookups)
Properties:
Extendible hashing (dynamic growth)
Global depth + local depth per bucket
No full table rehashing needed
Use Cases:
Position lookups: (session_id, row, col) → cell_id (O(1))
Player name lookups: player_name → session_id (O(1))
Operations:
search(key) → O(1) average → returns record_id
insert(key, value) → O(1) average, may trigger bucket split
Bucket splits when full (directory may grow)

Composite Key Hashing: For (session_id, row, col):
hash = hash(session_id) XOR (hash(row) << 1) XOR (hash(col) << 2)
B-Tree (Graph Traversal)
Properties:
Simplified B-Tree (data in all nodes)
Order: 64
Use Cases:
Clue reference graph: source_cell_id → list of target cells
Efficiently find all clues constraining a cell
Operations:
search(key) → O(log n)
insert(key, value) → O(log n)
Layer 4: Table Manager
Responsibilities:
Schema management
Record serialization/deserialization
Heap file management (unordered data storage)
Index coordination
Record Format (Variable Length):
[record_header: 4 bytes]
- record_size: 2 bytes
- num_fields: 2 bytes
[null_bitmap: variable]
[field_offsets: num_fields * 2 bytes]
[field_data: variable]
Key Operations:
insertRecord(table, record) → record_id
getRecord(table, record_id) → Record
updateRecord(table, record_id, record)

deleteRecord(table, record_id)
createIndex(table, columns, type)
Layer 5: Query Engine
Responsibilities:
Parse SQL-like queries
Select optimal index
Execute query plan
Return results
Supported Queries:
INSERT INTO table VALUES (...)
SELECT * FROM table WHERE condition
UPDATE table SET ... WHERE condition
DELETE FROM table WHERE condition
Index Selection Logic:
Exact match (WHERE x = 5) → prefer Hash Table
Range query (WHERE x BETWEEN 3 AND 7) → prefer B+ Tree
Join/graph traversal → prefer B-Tree
No suitable index → full table scan
Game Logic Engine
Puzzle Generator
Goal: Create solvable, unique-solution puzzles.
Algorithm:
Randomly assign TRUE/FALSE to all 100 cells
Generate clues based on actual solution:
Select random cells to contain clues
Analyze current state and generate truthful clue
Clue types depend on difficulty
Verify puzzle has unique solution (SAT solver or constraint propagation)
If not solvable/unique, retry (max 10 attempts)
Difficulty Levels:
Easy: Simple row/column counts, 1 clue type per cell, 40-50 reveals needed

Medium: Multiple clue types, region clues, 25-35 reveals needed
Hard: Complex patterns, relative clues, 15-20 reveals needed
Expert: Minimal clues, requires multi-step logic chains, 10-15 reveals needed
Clue Generation Examples:
Row Count:
Count TRUE cells in row X
Generate: "Exactly N cells in Row X are TRUE"
Corner:
Count FALSE corners
Generate: "Exactly N corners are FALSE"
Adjacent:
Check all adjacent cells of (r,c)
Generate: "All adjacent cells are TRUE" or "2 of 8 surrounding cells are FALSE"
Constraint Validator
Goal: Check if player deductions create logical contradictions.
Process:
Player submits: cell (5,7) = TRUE
Temporarily apply this deduction
Get all clues that reference this cell
For each clue, check if still satisfiable:
Row count: "Exactly 3 in Row 5 are TRUE"
Already have 3 TRUE deductions in Row 5?
This would make 4 → CONTRADICTION!
If contradiction found, reject deduction
Otherwise, accept and update database
Clue Satisfiability Check:
Track known TRUE count, known FALSE count, unknown count
Check against constraint (exactly/at_least/at_most)
Return: satisfiable / contradiction / already violated
Hint System

Goal: Help players find solvable cells.
Algorithm:
Find all unrevealed, un-deduced cells
For each cell, count how many clues constrain it
Return cell with most constraints (most information available)
List all relevant clues for that cell
Example Hint:
"Look at cell (6,7). It's constrained by 3 clues:
- Cell (0,0): Only 1 corner is FALSE
- Cell (5,5): Exactly 3 in Row 5 are TRUE
- Cell (6,6): All adjacent cells are TRUE"
Win Condition
Player wins when:
All 100 cells have been deduced (player_deduction is not NULL)
All deductions match actual_truth_value (100% accuracy)
On win:
Stop timer
Calculate completion time
Add entry to leaderboard
Show congratulations modal
API Specification (RESTful)
Endpoint 1: POST /api/game/new
Create new game session.
Request:
json
{
"player_name": "Alice",
"difficulty": "medium",
"grid_size": 10
}

Response:
json
{
"session_id": 123,
"grid_size": 10,
"total_cells": 100,
"status": "active"
}
Endpoint 2: POST /api/game/reveal
Reveal a cell.
Request:
json
{
"session_id": 123,
"row": 5,
"col": 7
}
Response:
json
{
"cell_id": 57,
"truth_value": true,
"clue_text": "Exactly 3 cells in Row 5 are TRUE",
"clue_type": "row_count"
}
Endpoint 3: POST /api/game/deduce
Submit player deduction.
Request:
json
{
"session_id": 123,
"cell_id": 42,

"deduced_value": true
}
Response (Success):
json
{
"success": true,
"contradictions": []
}
Response (Contradiction):
json
{
"success": false,
"contradictions": [
{
"clue_text": "Exactly 3 in Row 5 are TRUE",
"reason": "This would create 4 TRUE cells in Row 5"
}
]
}
Endpoint 4: POST /api/game/verify
Check all deductions.
Request:
json
{
"session_id": 123
}
Response:
json
{
"total_deduced": 90,
"correct_count": 87,
"incorrect_count": 3,
"completion_percentage": 96.7,
"is_won": false,

"wrong_cells": [
{"cell_id": 12, "row": 1, "col": 2, "correct_value": false}
]
}
Endpoint 5: GET /api/game/hint
Get hint for next move.
Query: ?session_id=123
Response:
json
{
"suggested_cell": {"row": 6, "col": 7},
"reason": "This cell has 3 clues constraining it",
"relevant_clues": [
"Cell (0,0): Only 1 corner is FALSE",
"Cell (5,5): Exactly 3 in Row 5 are TRUE"
]
}
Endpoint 6: GET /api/leaderboard
Get top players.
Query: ?difficulty=medium&limit=10
Response:
json
{
"difficulty": "medium",
"entries": [
{
"rank": 1,
"player_name": "Alice",
"completion_time": 450,
"total_moves": 25,
"accuracy_percent": 98.5
}
]
}

Frontend GUI Structure
Main Components
1. Grid Display (10×10)
Each cell: 60×60 pixels
States: unrevealed (gray), revealed (green), deduced-true (blue), deduced-false (red),
confirmed-correct (bright green), confirmed-wrong (bright red)
Click unrevealed → reveal
Click revealed → open deduction modal
2. Side Panel
Stats Box: revealed count, deduced count, accuracy, timer
Clue List: scrollable list of all revealed clues
Controls: hint button, verify button, new game button
3. Modals
New Game Modal: player name input, difficulty selector
Cell Reveal Modal: shows truth value + clue text
Deduction Modal: TRUE/FALSE/CLEAR buttons
Win Modal: shows completion time, stats, leaderboard rank
4. Header
Game title
Player name
Difficulty
Timer (MM:SS)
Implementation Phases
PHASE 1: Storage Engine Foundation (Weeks 1-3)
Goal: Implement persistent page-based storage with buffer pool caching.
Deliverables:
1. Disk Manager
Single database file management
Page allocation/deallocation (8KB pages)
Read/write operations with proper error handling
Free page list management (bitmap or linked list)

Database header page (page 0) with metadata
2. Page Structure
Page header (128 bytes): page_id, page_type, num_records, free_space, checksum
Slotted page layout for variable-length records
Slot array grows from bottom, records from top
3. Buffer Pool Manager
Fixed-size pool (100 pages = 800 KB)
Page table: page_id → frame_id mapping
Pin/unpin mechanism
Dirty page tracking
4. LRU Replacer
Doubly-linked list for LRU order
Hash map for O(1) access
victim() method to evict least recently used unpinned page
Testing Phase 1:
Test page persistence (write, close, reopen, read)
Test buffer pool hit rate (should be >80%)
Test LRU eviction correctness
Benchmark: 10,000 random page accesses
Verify no memory leaks
Success Criteria:
Database file persists across restarts
Buffer pool hit rate >80%
All unit tests pass
No memory leaks (use Valgrind)
PHASE 2: Index Structures (Weeks 3-6)
Goal: Implement B+ Tree, Hash Table, and B-Tree with full CRUD operations.
Deliverables:
1. B+ Tree Implementation
Internal nodes: keys + child pointers

Leaf nodes: keys + values (record IDs) + next pointer
Insert with split propagation
Delete with merge/borrow from sibling
Range search via leaf node links
Order: 128 (fits in 8KB page)
2. Hash Table Implementation
Extendible hashing with directory
Global depth and local depth tracking
Bucket split (not full table rehash)
Directory doubling when needed
Composite key support for (session_id, row, col)
3. B-Tree Implementation
Simplified B-Tree (data in all nodes)
Insert and search operations
Used for clue reference graph traversal
4. Index Manager
Create/drop indexes
Index type selection logic
Multi-index coordination for single table
Testing Phase 2:
Insert 100,000 keys in each structure
Test search correctness (all keys found)
Test range queries (B+ Tree)
Test delete operations (keys properly removed)
Benchmark performance comparison:
B+ Tree search: O(log n)
Hash Table search: O(1) average
Measure actual times
Success Criteria:
All three structures handle 100K+ entries
B+ Tree range queries work correctly
Hash Table O(1) search verified
Split/merge operations work correctly
Performance meets expectations

PHASE 3: Table Management & Query Engine (Weeks 6-8)
Goal: Build table abstraction and basic SQL-like query processing.
Deliverables:
1. Schema Definition
Define all 5 tables (GAME_SESSIONS, GRID_CELLS, etc.)
Column types: INT, VARCHAR, BOOLEAN, TIMESTAMP, ENUM
Primary keys and foreign keys
Index specifications per table
2. Record Management
Variable-length record serialization
Null bitmap for nullable fields
Field offset array for quick access
Insert/update/delete operations
3. Heap File Manager
Unordered data storage
Free space tracking per page
Record ID (RID) = (page_id, slot_num)
Page compaction when fragmented
4. Query Engine
Simple SQL parser (INSERT, SELECT, UPDATE, DELETE, WHERE)
Query plan: scan type (index vs sequential), filter conditions
Index selection based on query type
Result set generation
5. Index Integration
Automatic index updates on INSERT/UPDATE/DELETE
Multi-index support (primary + secondary indexes)
Index selection for WHERE clauses
Testing Phase 3:
Create all 5 database tables
Insert 1,000 game sessions

Insert 100,000 grid cells
Test queries:
SELECT by primary key (hash index)
SELECT with range (B+ Tree)
SELECT with JOIN (clue references)
Test UPDATE and DELETE with cascading index updates
Verify referential integrity (foreign keys)
Success Criteria:
All tables created successfully
INSERT/UPDATE/DELETE work correctly
Indexes automatically maintained
Complex queries return correct results
Query performance uses appropriate indexes
PHASE 4: Game Logic & Backend API (Weeks 8-10)
Goal: Implement puzzle generation, game mechanics, and RESTful API.
Deliverables:
1. Puzzle Generator
Random solution generation (100 cells)
Clue generation algorithm:
Row/column count clues
Corner clues
Adjacent clues
Region clues (for harder difficulties)
Pattern clues (expert only)
Solvability verification:
Constraint propagation algorithm
Ensure unique solution
Retry if not solvable (max 10 attempts)
Difficulty calibration (easy/medium/hard/expert)
2. Constraint Validator
Evaluate clue satisfiability
Detect contradictions when player makes deduction
Track known TRUE/FALSE/unknown cells per clue
Return contradiction reason if invalid
3. Game State Manager

Create new game (call puzzle generator)
Reveal cell (update database, return clue)
Submit deduction (validate, update if valid)
Check win condition (all cells correct)
Update game statistics
4. Hint Generator
Find cell with most constraints
Collect all clues referencing that cell
Generate helpful hint text
5. Leaderboard Manager
Add completed game to leaderboard
Query top N players by difficulty
Calculate rankings
6. HTTP API Server
Use C++ HTTP library (Crow or cpp-httplib)
Implement all 6 endpoints (new game, reveal, deduce, verify, hint, leaderboard)
JSON request/response parsing
Error handling and validation
CORS headers for frontend access
Testing Phase 4:
Generate 100 puzzles, verify all are solvable
Test each difficulty level playability
Test constraint validator with known contradictions
Test all API endpoints with Postman/curl
Simulate complete game playthrough via API
Load test: 10 concurrent games
Success Criteria:
100% of generated puzzles are solvable with unique solutions
Constraint validator catches all contradictions
Win condition correctly identifies completion
All API endpoints return correct data
Server handles concurrent requests
Game is playable end-to-end via API

PHASE 5: Frontend GUI (Weeks 10-11)
Goal: Build interactive web interface for playing the game.
Deliverables:
1. HTML Structure
Game header (title, player info, timer)
Main container:
Grid panel (10×10 cell grid)
Side panel (stats, clues, controls)
Modals:
New game modal (player name, difficulty)
Cell reveal modal (truth value, clue)
Deduction modal (TRUE/FALSE buttons)
Win modal (stats, leaderboard)
2. CSS Styling
Dark theme (modern look)
Responsive grid (scales to screen size)
Cell states with distinct colors:
Unrevealed: dark gray
Revealed: green with truth badge
Deduced TRUE: blue border
Deduced FALSE: red border
Confirmed correct: bright green
Confirmed wrong: bright red with X
Hover effects and animations
Modal overlays with backdrop blur
3. JavaScript Application Logic
Game initialization
API integration (fetch calls to backend)
Event handlers:
Click unrevealed cell → call /api/game/reveal
Click revealed cell → show deduction modal
Submit deduction → call /api/game/deduce
Hint button → call /api/game/hint, highlight cell
Verify button → call /api/game/verify, show results
New game button → show new game modal
UI updates:

Update cell appearance based on state
Add clues to side panel
Update stats in real-time
Timer countdown
State management:
Track session_id
Track grid state
Track revealed clues
4. User Experience Features
Loading indicators during API calls
Error messages for network failures
Confirmation dialogs for actions
Visual feedback (cell highlight, animations)
Keyboard shortcuts (optional)
Tutorial/help modal (optional)
5. Testing & Polish
Cross-browser testing (Chrome, Firefox, Safari, Edge)
Mobile responsive design
Accessibility (keyboard navigation, ARIA labels)
Performance optimization (minimize API calls)
Bug fixes and edge cases
Testing Phase 5:
Manual playtesting by 5+ users
Test all interactions (reveal, deduce, verify, hint)
Test error handling (network failures)
Test on different screen sizes
Verify timer accuracy
Check leaderboard display
