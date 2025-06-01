<script lang="ts">
    import RemotesComp from '$lib/components/RemotesComp.svelte';

    export let data;
</script>

<div
	class="mx-0 my-1 flex flex-col space-y-4
     sm:mx-8 sm:my-8"
>
    <RemotesComp restPath="rest/spin-seq/remotes" />
</div>
