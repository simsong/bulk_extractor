auto taskFuture = MyNamespace::DefaultThreadPool::submitJob([]()
{
    lengthyProcess();
});

auto taskFuture2 = MyNamespace::DefaultThreadPool::submitJob([](int a, float b)
{
    lengthyProcessWithArguments(a, b);
}, 5, 10.0f);
