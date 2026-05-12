Compiling
 g++ main.cpp Parser.cpp Translator.cpp -o sql_translator 
 ./sql_translator



SELECT sname FROM Student;

SELECT * FROM Student WHERE age > 20;

SELECT sid, sname FROM Student WHERE age BETWEEN 18 AND 22;

SELECT sid FROM Student WHERE age > 18 AND dept_id = 10;

SELECT sid, sname FROM Student WHERE dept_id = 10 OR dept_id = 20;

SELECT s.sid, s.sname, d.dept_name
FROM Student s
JOIN Department d ON s.dept_id = d.dept_id;

SELECT s.sname, d.dept_name
FROM Student s
JOIN Department d ON s.dept_id = d.dept_id
WHERE d.dept_name = 'CSE';

SELECT s.sname, c.cname, e.grade
FROM Student s
JOIN Enroll e ON s.sid = e.sid
JOIN Course c ON e.cid = c.cid;

SELECT i.iname, c.cname, t.semester
FROM Instructor i
JOIN Teaches t ON i.iid = t.iid
JOIN Course c ON t.cid = c.cid
WHERE i.dept_id = c.dept_id;

SELECT s1.sid, s2.sid
FROM Student s1
JOIN Student s2 ON s1.dept_id = s2.dept_id
WHERE s1.sid <> s2.sid;

SELECT sid FROM Student WHERE dept_id = 10
UNION
SELECT sid FROM Student WHERE dept_id = 20;

SELECT sid FROM Enroll WHERE grade = 'A'
INTERSECT
SELECT sid FROM WorksOn WHERE hours >= 10;

SELECT sid FROM Student
EXCEPT
SELECT sid FROM Enroll;

SELECT s.sid
FROM Student s JOIN Department d ON s.dept_id = d.dept_id
WHERE d.dept_name = 'CSE'
UNION
SELECT s.sid
FROM Student s JOIN Department d ON s.dept_id = d.dept_id
WHERE d.dept_name = 'ECE';

(
  SELECT sid FROM Enroll WHERE grade = 'A'
  UNION
  SELECT sid FROM Enroll WHERE grade = 'B'
)
EXCEPT
SELECT sid FROM WorksOn WHERE hours < 5;

SELECT s.sid, s.sname, d.dept_name, c.cnameFROM Student s JOIN Department d ON s.dept_id = d.dept_id JOIN Enroll e ON s.sid = e.sid JOIN Course c ON e.cid = c.cid WHERE c.dept_id = d.dept_id

SELECT s.sname, i.iname, c.cname, t.semester
FROM Student s
JOIN Department d ON s.dept_id = d.dept_id
JOIN Course c ON c.dept_id = d.dept_id
JOIN Teaches t ON t.cid = c.cid
JOIN Instructor i ON i.iid = t.iid
WHERE d.dept_name = 'CSE';

Assignment 2
Write a program which shall read an input file containing statistics above a database with at least five table and suggest the most cost effective join method for join operation between any given two relations. Your program shall compute and display the cost for all join methods applicable for those two relations. You may decide on the structure of the input file for storing that statistics above the database.

selection operation and external sorting

Convert SQL queries into equivalent Relational Algebras.