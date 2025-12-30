<script lang="ts">
    import RemotesRestComp from "$lib/components/RemotesRestComp.svelte";
    import ESPNowConsoleRestComp from "$lib/components/ESPNowConsoleRestComp.svelte";
</script>

<RemotesRestComp />
<ESPNowConsoleRestComp restPath="/rest/espnow" />