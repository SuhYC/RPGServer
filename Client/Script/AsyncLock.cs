using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public sealed class AsyncLock
{
    private readonly SemaphoreSlim _semaphore = new SemaphoreSlim(1, 1);

    /// <summary>
    /// »ç¿ë¹ý :
    /// private static readonly _asynclock = new AsyncLock(); <br/>
    /// using (await _asynclock.LockAsync()) { ... }
    /// </summary>
    /// <returns></returns>
    public async Task<IDisposable> LockAsync()
    {
        await _semaphore.WaitAsync();
        return new Handler(_semaphore);
    }

    private sealed class Handler : IDisposable
    {
        private readonly SemaphoreSlim _semaphore;
        private bool _disposed = false;

        public Handler(SemaphoreSlim semaphore)
        {
            _semaphore = semaphore;
        }

        public void Dispose()
        {
            if(!_disposed)
            {
                _semaphore.Release();
                _disposed = true;
            }
        }
    }
}
