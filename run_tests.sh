#!/usr/bin/env bash
# 一键验证：容器内逐个运行 exploit，全部拿到 shell 即为通过。
# 用法：bash run_tests.sh
set -u
pass=0; fail=0
for lab in lab-01-ret2text lab-02-ret2libc lab-03-uaf lab-04-tcache; do
    echo "===== $lab ====="
    if (cd "$lab" && python3 exploit.py >/tmp/"$lab".log 2>&1); then
        echo "PASS"
        pass=$((pass+1))
    else
        echo "FAIL  (log: /tmp/$lab.log)"
        tail -5 /tmp/"$lab".log
        fail=$((fail+1))
    fi
done
echo "===== result: $pass passed, $fail failed ====="
[ "$fail" = "0" ]
