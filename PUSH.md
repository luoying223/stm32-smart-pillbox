# 推送到 GitHub（只需要执行一次）

仓库已经在本地建好并完成首次提交（分支 `main`）。联网恢复后，在本目录执行：

```powershell
# 1. 登录（浏览器授权）
gh auth login

# 2. 建仓库并推送（公开）
gh repo create stm32-smart-pillbox --public --source . --remote origin --push
```

推送成功后地址为：https://github.com/luoying223/stm32-smart-pillbox

## 如果不用 gh，用网页方式

1. 打开 https://github.com/new
2. Repository name 填 `stm32-smart-pillbox`，选择 **Public**，不要勾选 README（本地已有）
3. 在该页面复制 `git remote add origin ...` 那两行命令，在本目录执行：

```powershell
git remote add origin https://github.com/luoying223/stm32-smart-pillbox.git
git push -u origin main
```
