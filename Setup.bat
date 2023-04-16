if not exist Solution\ (
    mkdir .\Solution
)
cd .\Solution
cmake .. -G "Visual Studio 17 2022" -A x64 -DPLATFORM:STRING=WINDOWS -DBRANCH=master -DGOOGLE_TEST_COMMIT_ID=12a5852e451baabc79c63a86c634912c563d57bc